// armaTypes.hpp
// Defines types that are present in ARMA
#pragma once
#include <string_view>
#include <string>
#include <algorithm>
#include <vector>
#include <type_traits>
#include <memory>
#include <charconv>

namespace potato {
	enum class variableType : uint8_t {
		UNKNOWN,
		NUMBER,
		STRING,
		BOOLEAN,
		ARRAY
	};

    struct baseARMAVariable;

    inline std::string_view getTypeString(variableType type) {
		switch (type) {
			case variableType::UNKNOWN:
				return "Unknown Type";
				break;
			case variableType::NUMBER:
				return "Number";
				break;
			case variableType::STRING:
				return "String";
				break;
			case variableType::BOOLEAN:
				return "Boolean";
				break;
			case variableType::ARRAY:
				return "Array";
				break;
			default:
				return "Type is not defined";
				break;
		}
	}

    template<typename T>
    constexpr variableType getVariableType() {
        if constexpr (std::is_arithmetic<T>::value) {
            return variableType::NUMBER;
        }

        if constexpr (std::is_same<T, std::string>::value) {
            return variableType::STRING;
        }

        if constexpr (std::is_same<T, bool>::value) {
            return variableType::BOOLEAN;
        }

        if constexpr (std::is_same<T, std::vector<std::unique_ptr<baseARMAVariable>>>::value) {
            return variableType::ARRAY;
        }

        return variableType::UNKNOWN;
    }

	inline variableType getTypeFromString(std::string_view data) {
		if (data.empty()) {
			return variableType::UNKNOWN;
		}

		if (data == "true" || data == "false") {
			return variableType::BOOLEAN;
		}

		if (data[0] == '[') {
			return variableType::ARRAY;
		}

		if (data[0] == '"') {
			return variableType::STRING;
		}

		return variableType::NUMBER;
	}

    struct baseARMAVariable {
        const variableType type = variableType::UNKNOWN;
        virtual std::string toString() const { return "base"; }
        virtual void fromString(std::string_view str) = 0;

        virtual void *getDataPointer() const { return dataPtr; }
        virtual std::intptr_t getDataPointerSize() const = 0;

        template<typename T>
        bool convert(T &output) {
            if (type != getVariableType<T>()) {
                return false;
            }
            output = *static_cast<T*>(dataPtr);
            return true;
        }

        bool set(void *data, variableType desiredType) {
            if (desiredType != type) {
                return false;
            }
            switch (type) {
                case variableType::UNKNOWN:
                    break;
                case variableType::NUMBER: {
                        double *dataDouble = reinterpret_cast<double*>(dataPtr);
                        *dataDouble = *static_cast<double*>(data);
                    }
                    break;
                case variableType::STRING: {
                        std::string *dataString = reinterpret_cast<std::string*>(dataPtr);
                        *dataString = *static_cast<std::string*>(data);
                    }
                    break;
                case variableType::BOOLEAN: {
                        bool *dataBool = reinterpret_cast<bool*>(dataPtr);
                        *dataBool = *static_cast<bool*>(data);
                    }
                    break;
                case variableType::ARRAY: {
                        //std::vector<std::unique_ptr<baseARMAVariable>> *dataArray = reinterpret_cast<std::vector<std::unique_ptr<baseARMAVariable>>*>(dataPtr);
                        //*dataArray = *static_cast<std::vector<std::unique_ptr<baseARMAVariable>>*>(data);
                    }
                    break;
                default:
                    return false;
                    break;
            }
            return true;
        }

        protected:
            void *dataPtr = nullptr;
	};

	template <variableType t_type>
	struct armaVariable : baseARMAVariable {
		std::string toString() const override final { return "unknown"; }
        void fromString(std::string_view str) override final {}
        std::intptr_t getDataPointerSize() const override final { return 0; }
    };

	template <>
	struct armaVariable<variableType::NUMBER> : baseARMAVariable {
		double data = 0.0;
		std::string toString() const override final { return std::to_string(data); }
        void fromString(std::string_view str) override final {
            std::from_chars(str.data(), str.data() + str.size(), data);
        }

        std::intptr_t getDataPointerSize() const override final { return sizeof(data); }

		armaVariable() {
			dataPtr = reinterpret_cast<std::uint8_t*>(this) + offsetof(armaVariable<variableType::NUMBER>, data);
            *const_cast<variableType*>(&type) = variableType::NUMBER;
		}
	};

	template <>
	struct armaVariable<variableType::STRING> : baseARMAVariable {
		std::string data = "";
		std::string toString() const override final { return data; }
        void fromString(std::string_view str) override final {
            data = str;
        }

        void *getDataPointer() const override final { return const_cast<char*>(data.c_str()); }
        std::intptr_t getDataPointerSize() const override final { return data.size(); }

        armaVariable() {
            dataPtr = reinterpret_cast<std::uint8_t*>(this) + offsetof(armaVariable<variableType::STRING>, data);
            *const_cast<variableType*>(&type) = variableType::STRING;
        }
	};

	template <>
	struct armaVariable<variableType::BOOLEAN> : baseARMAVariable {
		bool data = false;
		std::string toString() const override final { return data ? "true" : "false"; }
        void fromString(std::string_view str) override final {
            data = str == "true";
        }

        std::intptr_t getDataPointerSize() const override final { return sizeof(data); }

        armaVariable() {
            dataPtr = reinterpret_cast<std::uint8_t*>(this) + offsetof(armaVariable<variableType::BOOLEAN>, data);
            *const_cast<variableType*>(&type) = variableType::BOOLEAN;
        }
	};

	template <>
	struct armaVariable<variableType::ARRAY> : baseARMAVariable {
		std::vector<std::unique_ptr<baseARMAVariable>> data = {};
		std::string toString() const override final { return "not implemented"; }
        void fromString(std::string_view str) override final {
            // fukin who knows man
        }

        void *getDataPointer() const override final { return nullptr; }
        std::intptr_t getDataPointerSize() const override final { return 0; }

        armaVariable() {
            dataPtr = reinterpret_cast<std::uint8_t*>(this) + offsetof(armaVariable<variableType::ARRAY>, data);
            *const_cast<variableType*>(&type) = variableType::ARRAY;
        }
	};

    inline std::unique_ptr<baseARMAVariable> getARMAVariableFromType(variableType wantedType) {
        #define RETURN_APPLICABLE_TYPE(type) \
        if (wantedType == type) {\
            return std::make_unique<armaVariable<type>>();\
        }

        RETURN_APPLICABLE_TYPE(variableType::UNKNOWN);
        RETURN_APPLICABLE_TYPE(variableType::NUMBER);
        RETURN_APPLICABLE_TYPE(variableType::STRING);
        RETURN_APPLICABLE_TYPE(variableType::BOOLEAN);
        RETURN_APPLICABLE_TYPE(variableType::ARRAY);

        #undef RETURN_APPLICABLE_TYPE
    }
}
