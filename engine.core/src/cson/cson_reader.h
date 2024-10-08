
#pragma once
#include "containers/queue.h"
#include "cson_types.h"
#include "defines.h"

namespace C3D
{
    class C3D_API CSONReader
    {
    public:
        CSONObject Read(const String& input);
        CSONObject ReadFromFile(const String& path);

    private:
        CSONToken TokenizeDefault(char c, u32& index, u32& line);

        void Tokenize();

        bool ParseKeyOrEndOfObject(const CSONToken& token);
        bool ParseColon(const CSONToken& token);
        bool ParseValue(const CSONToken& token);
        bool ParseCommaOrEndOfObject(const CSONToken& token);
        bool ParseArrayOrObject(const CSONToken& token);
        bool ParseArrayValue(const CSONToken& token);
        bool ParseNegativeArrayValue(const CSONToken& token);
        bool ParseArraySeparatorOrEnd(const CSONToken& token);
        bool ParseEndOfFile(const CSONToken& token);

        bool ParseError(const CSONToken& token, const char* expected);

        CSONObject ParseTokens();

        Queue<CSONToken> m_tokens;

        CSONObject* m_pCurrentObject = nullptr;

        CSONTokenizeMode m_tokenizeMode;
        CSONParseMode m_parseMode;

        const String* m_pInput = nullptr;
    };
}  // namespace C3D
