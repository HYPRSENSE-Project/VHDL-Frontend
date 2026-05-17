// SPDX-License-Identifier: MIT
// Copyright (c) 2026 MINRES Technologies GmbH
// Copyright (c) 2015 Nic30

#include "encoding_conversions.h"
#include <Exceptions.h>
#include <codecvt>
#include <locale>

namespace parser {

std::string iso_8859_1_to_utf8(const std::string& str) {
    std::string strOut;
    strOut.reserve(str.size());
    for(auto it = str.begin(); it != str.end(); ++it) {
        uint8_t ch = *it;
        if(ch < 0x80) {
            strOut.push_back(ch);
        } else {
            strOut.push_back(0xc0 | ch >> 6);
            strOut.push_back(0x80 | (ch & 0x3f));
        }
    }
    return strOut;
}

std::string _to_utf8(const std::string& str, encoding enc) {
    if(encoding::UTF_8 == enc) {
        return str;
    } else if(encoding::UTF_16 == enc) {
        auto& _str = reinterpret_cast<const std::u16string&>(str);
// because of bug in MSVC
// https://stackoverflow.com/questions/32055357/visual-studio-c-2015-stdcodecvt-with-char16-t-or-char32-t
#if defined(_MSC_VER) && _MSC_VER >= 1900 && _MSC_VER < 2000
        std::wstring_convert<std::codecvt_utf8_utf16<int16_t>, int16_t> convert;
        auto p = reinterpret_cast<const int16_t*>(_str.data());
        return convert.to_bytes(p, p + _str.size());
#else
        std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;
        return convert.to_bytes(_str);
#endif
    } else if(encoding::ISO_8859_1 == enc) {
        return iso_8859_1_to_utf8(str);
    } else {
        throw std::runtime_error("iParserContainer implemented only for utf-8, "
                                 "utf-16 and iso-8859-1");
    }
}

antlr4::ANTLRInputStream ANTLRFileStream_with_encoding(const std::filesystem::path& file_name, encoding enc) {
    std::ifstream ifs;
    std::string str;
    ifs.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    ifs.open(file_name);

    // Sets position to the end of the file.
    ifs.seekg(0, std::ios::end);

    // Reserves memory for the file.
    str.reserve(ifs.tellg());

    // Sets position to the start of the file.
    ifs.seekg(0, std::ios::beg);

    // Sets contents of 'str' to all characters in the file.
    str.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());

    try {
        str = _to_utf8(str, enc);
        antlr4::ANTLRInputStream input_stream(str);
        input_stream.name = file_name.u8string();
        return input_stream;
    } catch(const antlr4::IllegalArgumentException& e) {
        if(enc != encoding::UTF_8)
            throw;
        antlr4::ANTLRInputStream input_stream(iso_8859_1_to_utf8(str));
        input_stream.name = file_name.u8string();
        return input_stream;
    } catch(const std::range_error& e) {
        throw std::range_error(std::string("Invalid character/encoding in ") + file_name.u8string() + " file");
    }
}

} // namespace parser
