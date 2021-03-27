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
#include <stack>

namespace potato {
    // The variable types we support coming from ARMA
	enum class variableType : uint8_t {
		UNKNOWN,
		NUMBER,
		STRING,
		BOOLEAN,
		ARRAY
	};

    struct baseARMAVariable;

    // Return the type as a string
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

    // Return the ARMA variableType from C++ type
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

    // Return the ARMA variableType from a string representation of data
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

    // Return a pointer to ARMA type representation from ARMA type enum
    inline std::unique_ptr<baseARMAVariable> getARMAVariableFromType(variableType wantedType);

    // The base class for all ARMA types within C++
    // We use polymorphism to be able to have templated types that behave differently at compile time
    // that means that all of these variables are on the heap, but that shouldnt matter too much
    struct baseARMAVariable {
        const variableType type = variableType::UNKNOWN;

        // Returns the variable as a string
        virtual std::string toString() const { return "base"; }

        // Parses string and converts to data
        virtual void fromString(std::string_view str) = 0;

        // Return this type in memory
        virtual void *getDataPointer() const { return dataPtr; }

        // Return how many bytes this type uses
        virtual std::intptr_t getDataPointerSize() const = 0;

        // convert the data stored inside to whatever we want to output
        // prevent mapping to wrong type by checking the type coming in
        template<typename T>
        bool convert(T &output) {
            if (type != getVariableType<T>()) {
                return false;
            }
            output = *static_cast<T*>(dataPtr);
            return true;
        }

        // set the internal data to pointed data
        // this isn't templated to allow setting memory buffers to the internal data
        bool set(void *data, variableType desiredType, std::size_t expectedSize);

        protected:
            void *dataPtr = nullptr;
	};

	template <variableType t_type>
	struct armaVariable : baseARMAVariable {
		std::string toString() const override final { return "unknown"; }
        void fromString(std::string_view str) override final {}
        std::intptr_t getDataPointerSize() const override final { return 0; }
    };

    using armaNumber = armaVariable<variableType::NUMBER>;
    using armaString = armaVariable<variableType::STRING>;
    using armaBool = armaVariable<variableType::BOOLEAN>;
    using armaArray = armaVariable<variableType::ARRAY>;

	template <>
	struct armaVariable<variableType::NUMBER> : baseARMAVariable {
		double data = 0.0;
		std::string toString() const override final { return std::to_string(data); }
        void fromString(std::string_view str) override final {
            std::from_chars(str.data(), str.data() + str.size(), data);
        }

        std::intptr_t getDataPointerSize() const override final { return sizeof(data); }

		armaVariable() {
			dataPtr = &data;
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
            dataPtr = &data;
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
            dataPtr = &data;
            *const_cast<variableType*>(&type) = variableType::BOOLEAN;
        }
	};

	template <>
	struct armaVariable<variableType::ARRAY> : baseARMAVariable {
		std::vector<std::unique_ptr<baseARMAVariable>> data = {};
        std::vector<std::uint8_t> dataBuffer = {};

		std::string toString() const override final {
            std::string formattedString = "[";
            for (const auto &var : data) {
                formattedString += var->toString() + ", ";
            }

            // if we only have the starting bracket, don't delete it. Otherwise delete the trailing comma
            if (formattedString.size() > 1) {
                formattedString.erase(formattedString.end() - 1);
                formattedString.erase(formattedString.end() - 1);
            }

            formattedString += ']';
            return formattedString;
        }

        void fromString(std::string_view str) override final {
            int currentIndex = 1; // we know the first index is '['
            std::string dataString = "";
            bool inStringType = false;

            std::stack<armaArray*> arrayStack;
            arrayStack.push(this);

            using arrayVector = std::vector<std::unique_ptr<baseARMAVariable>>;

            // lambda to push values onto the vector. We need this twice in the parsing, and it is quite ugly so a lambda makes it nicer to read
            auto pushData = [] (arrayVector &dataVector, std::string_view data) {
                variableType varType = getTypeFromString(data);
                std::unique_ptr<baseARMAVariable> armaVariable = std::move(getARMAVariableFromType(varType));
                armaVariable->fromString(data);
                dataVector.push_back(std::move(armaVariable));
            };

            // parse array string. We have two states, we are either processing a string or we aren't.
            // If we aren't parsing a string, we read the values until we find a comma and then push it to the data vector
            // If we find the start of a new array, we push a new array onto the data vector
            // If we parse a string, we can't exit on a comma until we exit the string, so we process until we find an end quote
            while (!arrayStack.empty()) {
                if (currentIndex >= str.size()) {
                    throw std::exception("Overran string when creating array type");
                }

                if (str[currentIndex] == '[') {
                    dataString = "";

                    data.emplace_back(std::make_unique<armaVariable<variableType::ARRAY>>());
                    arrayStack.push(static_cast<armaVariable<variableType::ARRAY>*>(data.back().get()));
                } else if (str[currentIndex] == ']') {
                    if (dataString != "") {
                        pushData(arrayStack.top()->data, dataString);
                    }
                    dataString = "";

                    arrayStack.top()->toDataBuffer();
                    arrayStack.pop();
                } else {
                    if (inStringType) {
                        if (str[currentIndex] == '"') {
                            inStringType = false;
                        }
                        dataString += str[currentIndex];
                    }
                    else {
                        if (str[currentIndex] == ',') {
                            pushData(arrayStack.top()->data, dataString);
                            dataString = "";
                        } else if (str[currentIndex] != ' ') {
                            dataString += str[currentIndex];
                        }

                        if (str[currentIndex] == '"') {
                            inStringType = true;
                        }
                    }
                }
                currentIndex++;
            }
        }

        // copy internal variables to a byte array
        void toDataBuffer() {
            std::size_t variableCount = data.size();
            std::uint8_t *variableCountPtr = reinterpret_cast<std::uint8_t*>(&variableCount);

            dataBuffer.clear();
            dataBuffer.insert(dataBuffer.begin(), variableCountPtr, variableCountPtr + sizeof(variableCount));

            for (auto &variable : data) {
                std::uint8_t *dataPtr = reinterpret_cast<std::uint8_t*>(variable->getDataPointer());
                std::intptr_t dataPtrSize = variable->getDataPointerSize();
                variableType dataType = variable->type;

                std::uint8_t metaData[sizeof(dataPtrSize) + sizeof(dataType)] = {};
                std::memcpy(metaData, &dataPtrSize, sizeof(dataPtrSize));
                std::memcpy(metaData + sizeof(dataPtrSize), &dataType, sizeof(dataType));

                dataBuffer.insert(dataBuffer.end(), std::begin(metaData), std::end(metaData));
                dataBuffer.insert(dataBuffer.end(), dataPtr, dataPtr + variable->getDataPointerSize());
            }
        }

        // copy byte array to vector of arma types
        void fromDataBuffer() {
            data.clear();

            const std::uint8_t *dataPtr = dataBuffer.data();
            std::size_t variableCount = *reinterpret_cast<const std::size_t*>(dataPtr);

            std::uintptr_t offset = sizeof(std::size_t);

            for (std::size_t i = 0; i < variableCount; i++) {
                std::intptr_t dataSize = *reinterpret_cast<const std::intptr_t*>(dataPtr + offset);
                offset += sizeof(dataSize);

                variableType dataType = *reinterpret_cast<const variableType*>(dataPtr + offset);
                offset += sizeof(dataType);

                data.emplace_back(getARMAVariableFromType(dataType));
                data.back()->set(const_cast<std::uint8_t*>(dataPtr + offset), dataType, dataSize);

                offset += dataSize;
            }
        }

        void *getDataPointer() const override final { return const_cast<std::uint8_t*>(dataBuffer.data()); }
        std::intptr_t getDataPointerSize() const override final { return dataBuffer.size(); }

        armaVariable() {
            dataPtr = &data;
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

    inline bool baseARMAVariable::set(void *data, variableType desiredType, std::size_t expectedSize) {
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
                // Copy data from pointer to buffer, and then convert into string
                std::vector<char> inputString(expectedSize + 1);
                for (std::size_t i = 0; i < expectedSize; i++) {
                    inputString[i] = *(static_cast<std::uint8_t*>(data) + i);
                }
                inputString[expectedSize] = '\0';

                std::string *dataString = reinterpret_cast<std::string*>(dataPtr);
                *dataString = inputString.data();
            }
            break;
            case variableType::BOOLEAN: {
                bool *dataBool = reinterpret_cast<bool *>(dataPtr);
                *dataBool = *static_cast<bool*>(data);
            }
            break;
            case variableType::ARRAY: {
                // Copy data from buffer. This assumes that the data is setup through armaArray::toDataBuffer 
                armaArray *thisArray = static_cast<armaArray*>(this);
                std::uint8_t *dataBytePtr = static_cast<std::uint8_t*>(data);
                for (int i = 0; i < expectedSize; i++) {
                    thisArray->dataBuffer.push_back(dataBytePtr[i]);
                }
                thisArray->fromDataBuffer();
            }
            break;
            default:
                return false;
                break;
        }
        return true;
    }
}
