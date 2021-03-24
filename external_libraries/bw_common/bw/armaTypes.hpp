// armaTypes.hpp
// Defines types that are present in ARMA
#pragma once
#include <string_view>
#include <algorithm>

namespace potato {
	enum class variableType : uint8_t {
		UNKNOWN,
		NUMBER,
		STRING,
		BOOLEAN,
		ARRAY
	};

	std::string_view getTypeString(variableType type) {
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

	variableType typeResolver(std::string_view data) {
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
}
