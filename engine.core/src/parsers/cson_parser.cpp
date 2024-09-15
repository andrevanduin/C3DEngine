
#include "cson_parser.h"

#include "asserts/asserts.h"
#include "platform/file_system.h"
#include "string/string_utils.h"

namespace C3D
{
    CSONObject CSONParser::Parse(const String& input)
    {
        // Take a pointer to the input string
        m_pInput = &input;
        // Tokenize the input string
        Tokenize();
        // Parse those tokens into CSONObjects
        return ParseTokens();
    }

    CSONObject CSONParser::ParseFile(const String& path)
    {
        File file;
        if (!file.Open(path, FileModeRead))
        {
            ERROR_LOG("Failed to open CSON file: '{}'.", path);
            return CSONObject(CSONObjectType::Object);
        }

        String input;
        if (!file.ReadAll(input))
        {
            ERROR_LOG("Failed to read CSON file: '{}'.", path);
            return CSONObject(CSONObjectType::Object);
        }
        return Parse(input);
    }

    CSONToken CSONParser::TokenizeDefault(char c, u32& index, u32& line)
    {
        switch (c)
        {
            case ' ':
                // Switch to parsing whitespace
                m_tokenizeMode = CSONTokenizeMode::Whitespace;
                return CSONToken(CSONTokenType::Whitespace, index, line);
            case '#':
                // Switch to parsing comments
                m_tokenizeMode = CSONTokenizeMode::Comment;
                return CSONToken(CSONTokenType::Comment, index, line);
            case '\n':
                // Increment our current line number
                line++;
                return CSONToken(CSONTokenType::NewLine, index, line);
            case '"':
                // Switch to parsing string literals
                m_tokenizeMode = CSONTokenizeMode::StringLiteral;
                return CSONToken(CSONTokenType::StringLiteral, index, line);
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                // Switch to parsing numeric literals
                m_tokenizeMode = CSONTokenizeMode::NumericLiteral;
                return CSONToken(CSONTokenType::Integer, index, line);
            case '{':
                return CSONToken(CSONTokenType::OpenCurlyBrace, index, line);
            case '}':
                return CSONToken(CSONTokenType::CloseCurlyBrace, index, line);
            case ':':
                return CSONToken(CSONTokenType::Colon, index, line);
            case '[':
                return CSONToken(CSONTokenType::OpenSquareBrace, index, line);
            case ']':
                return CSONToken(CSONTokenType::CloseSquareBrace, index, line);
            case ',':
                return CSONToken(CSONTokenType::Comma, index, line);
            case '*':
                return CSONToken(CSONTokenType::OperatorAsterisk, index, line);
            case '+':
                return CSONToken(CSONTokenType::OperatorPlus, index, line);
            case '-':
                return CSONToken(CSONTokenType::OperatorMinus, index, line);
            case '/':
                return CSONToken(CSONTokenType::OperatorSlash, index, line);
            case 'f':
            case 'F':
                if (StringUtils::IEquals(m_pInput->Data() + index, "false", 5))
                {
                    auto token = CSONToken(CSONTokenType::Boolean, index, index + 4, line);
                    // Skip 4 characters for "false" because we will do another index++ after this
                    index += 4;
                    return token;
                }
                break;
            case 't':
            case 'T':
                if (StringUtils::IEquals(m_pInput->Data() + index, "true", 4))
                {
                    auto token = CSONToken(CSONTokenType::Boolean, index, index + 3, line);
                    // Skip 3 characters for "true" because we will do another index++ after this
                    index += 3;
                    return token;
                }
                break;
        }

        FATAL_LOG("Unsupported character found during tokenization: '{}'.", c);
        return CSONToken();
    }

    void CSONParser::Tokenize()
    {
        // We start in default tokenize mode
        m_tokenizeMode = CSONTokenizeMode::Default;
        // Clear our token queue
        m_tokens.Clear();

        u32 index = 0;
        u32 line  = 1;

        while (index < m_pInput->Size())
        {
            auto c = (*m_pInput)[index];

            switch (m_tokenizeMode)
            {
                case CSONTokenizeMode::Default:
                {
                    CSONToken token = TokenizeDefault(c, index, line);
                    m_tokens.Enqueue(token);
                }
                break;
                case CSONTokenizeMode::Comment:
                {
                    auto& token = m_tokens.PeekTail();
                    if (c != '\n')
                    {
                        // We have found another character in the comment so let's increment the end of the token
                        token.end++;
                    }
                    else
                    {
                        // We have found a newline which marks the end of this comment
                        m_tokenizeMode = CSONTokenizeMode::Default;
                        continue;
                    }
                }
                break;
                case CSONTokenizeMode::NumericLiteral:
                {
                    auto& token = m_tokens.PeekTail();
                    if (c >= '0' && c <= '9')
                    {
                        // We have found another numerical literal character so let's increment the end of token
                        token.end++;
                    }
                    else if (c == '.')
                    {
                        // We have found a dot, which means we are dealing with a double here
                        token.type = CSONTokenType::Float;
                        token.end++;
                    }
                    else
                    {
                        // No more numeric literals
                        m_tokenizeMode = CSONTokenizeMode::Default;
                        continue;
                    }
                }
                break;
                case CSONTokenizeMode::Whitespace:
                {
                    auto& token = m_tokens.PeekTail();
                    if (c == ' ')
                    {
                        // We have found more whitespace so let's increment the end of the token
                        token.end++;
                    }
                    else
                    {
                        // No more whitespace
                        m_tokenizeMode = CSONTokenizeMode::Default;
                        continue;
                    }
                }
                break;
                case CSONTokenizeMode::StringLiteral:
                {
                    auto& token = m_tokens.PeekTail();
                    token.end++;
                    if (c == '\"')
                    {
                        // We did found the closing '"' so we can stop parsing the string literal
                        m_tokenizeMode = CSONTokenizeMode::Default;
                    }
                }
                break;
            }

            index++;
        }

        // Make sure we end with a EndOfFile token to let the parser know when the file is at it's end
        m_tokens.Enqueue({ CSONTokenType::EndOfFile, index, index });
    }

    bool CSONParser::ParseKeyOrEndOfObject(const CSONToken& token)
    {
        if (token.type == CSONTokenType::StringLiteral)
        {
            // Next token should be a colon
            m_parseMode = CSONParseMode::Colon;
            // Add a named property to the current object (skip the starting and ending '"')
            m_pCurrentObject->properties.EmplaceBack(m_pInput->SubStr(token.start + 1, token.end));
            return true;
        }

        if (token.type == CSONTokenType::CloseCurlyBrace)
        {
            // We have found the end of the current object so check if we have a parent object
            if (m_pCurrentObject->parent)
            {
                // If we do we continue parsing for the parent
                m_pCurrentObject = m_pCurrentObject->parent;
                // We are expecting a comma
                m_parseMode = CSONParseMode::CommaOrEndOfObject;
            }
            else
            {
                // Otherwise we are done and we expect the end of the file
                m_parseMode = CSONParseMode::EndOfFile;
            }
            return true;
        }

        return ParseError(token, "string literal key or }");
    }

    bool CSONParser::ParseColon(const CSONToken& token)
    {
        if (token.type == CSONTokenType::Colon)
        {
            // Next up we should expect a value
            m_parseMode = CSONParseMode::Value;
            return true;
        }

        return ParseError(token, ":");
    }

    bool CSONParser::ParseValue(const CSONToken& token)
    {
        switch (token.type)
        {
            case CSONTokenType::Integer:
            {
                // Set the value to the last property (which we named in the ParseKey stage)
                m_pCurrentObject->properties.Last().value = m_pInput->SubStr(token.start, token.end + 1).ToI64();
                // Next up we expect a comma
                m_parseMode = CSONParseMode::CommaOrEndOfObject;
                return true;
            }
            case CSONTokenType::Float:
            {
                // Set the value to the last property (which we named in the ParseKey stage)
                m_pCurrentObject->properties.Last().value = m_pInput->SubStr(token.start, token.end + 1).ToF64();
                // Next up we expect a comma
                m_parseMode = CSONParseMode::CommaOrEndOfObject;
                return true;
            }
            case CSONTokenType::Boolean:
            {
                // Set the value to the last property (which we named in the ParseKey stage)
                m_pCurrentObject->properties.Last().value = m_pInput->SubStr(token.start, token.end + 1).ToBool();
                // Next up we expect a comma
                m_parseMode = CSONParseMode::CommaOrEndOfObject;
                return true;
            }
            case CSONTokenType::StringLiteral:
            {
                // Set the value to the last property (which we named in the ParseKey stage)
                m_pCurrentObject->properties.Last().value = m_pInput->SubStr(token.start + 1, token.end);
                // Next up we expect a comma
                m_parseMode = CSONParseMode::CommaOrEndOfObject;
                return true;
            }
            case CSONTokenType::OpenSquareBrace:
            {
                // The value provided appears to be an array so we create a new array object
                m_pCurrentObject->properties.Last().value = CSONArray(CSONObjectType::Array);
                // We take a pointer to the new array
                auto newCurrent = &std::get<CSONArray>(m_pCurrentObject->properties.Last().value);
                // Then we set it's parent to our current object
                newCurrent->parent = m_pCurrentObject;
                // Then we set the current object to the new array
                m_pCurrentObject = newCurrent;
                // Next up we expect array values (or for empty arrays a closing bracket)
                m_parseMode = CSONParseMode::ArrayValueAfterOpen;
                return true;
            }
            case CSONTokenType::OpenCurlyBrace:
            {
                // The value provided appears to be an object so we create a new object
                m_pCurrentObject->properties.Last().value = CSONObject(CSONObjectType::Object);
                // We take a pointer to the new object
                auto newCurrent = &std::get<CSONObject>(m_pCurrentObject->properties.Last().value);
                // Then we set it's parent to our current object
                newCurrent->parent = m_pCurrentObject;
                // Then we set the current object to the new object
                m_pCurrentObject = newCurrent;
                // Next up we expect a key inside of this object (or it's an empty object)
                m_parseMode = CSONParseMode::KeyOrEndOfObject;
                return true;
            }
            default:
                return ParseError(token, "a valid value");
        }
    }

    bool CSONParser::ParseCommaOrEndOfObject(const CSONToken& token)
    {
        if (token.type == CSONTokenType::Comma)
        {
            // We have found our comma so we should start parsing another key
            m_parseMode = CSONParseMode::KeyOrEndOfObject;
            return true;
        }

        if (token.type == CSONTokenType::CloseCurlyBrace)
        {
            // We have found the end of the object
            if (m_pCurrentObject->parent)
            {
                // We have a parent so we continue parsing that one
                m_pCurrentObject = m_pCurrentObject->parent;
                // Check if the parent was an object or an array
                if (m_pCurrentObject->type == CSONObjectType::Object)
                {
                    // We expect a comma or the end of that object next
                    m_parseMode = CSONParseMode::CommaOrEndOfObject;
                }
                else
                {
                    // We expect a comma or the end of the array next
                    m_parseMode = CSONParseMode::ArraySeparatorOrEnd;
                }
            }
            else
            {
                // No parent so we expect the end of the file
                m_parseMode = CSONParseMode::EndOfFile;
            }
            return true;
        }

        return ParseError(token, ",");
    }

    bool CSONParser::ParseArrayOrObject(const CSONToken& token)
    {
        if (token.type == CSONTokenType::OpenCurlyBrace)
        {
            // We are parsing an object
            m_pCurrentObject->type = CSONObjectType::Object;
            // and we expect a key next
            m_parseMode = CSONParseMode::KeyOrEndOfObject;
            return true;
        }

        if (token.type == CSONTokenType::OpenSquareBrace)
        {
            // We are parsing an array
            m_pCurrentObject->type = CSONObjectType::Array;
            // and expect array values next
            m_parseMode = CSONParseMode::ArrayValueAfterOpen;
            return true;
        }

        return ParseError(token, "{ or [");
    }

    bool CSONParser::ParseArrayValue(const CSONToken& token)
    {
        switch (token.type)
        {
            case CSONTokenType::Integer:
            {
                CSONValue value = m_pInput->SubStr(token.start, token.end + 1).ToI64();
                m_pCurrentObject->properties.EmplaceBack(value);
                m_parseMode = CSONParseMode::ArraySeparatorOrEnd;
                return true;
            }
            case CSONTokenType::Float:
            {
                CSONValue value = m_pInput->SubStr(token.start, token.end + 1).ToF64();
                m_pCurrentObject->properties.EmplaceBack(value);
                m_parseMode = CSONParseMode::ArraySeparatorOrEnd;
                return true;
            }
            case CSONTokenType::Boolean:
            {
                CSONValue value = m_pInput->SubStr(token.start, token.end + 1).ToBool();
                m_pCurrentObject->properties.EmplaceBack(value);
                m_parseMode = CSONParseMode::ArraySeparatorOrEnd;
                return true;
            }
            case CSONTokenType::StringLiteral:
            {
                CSONValue value = m_pInput->SubStr(token.start + 1, token.end);
                m_pCurrentObject->properties.EmplaceBack(value);
                m_parseMode = CSONParseMode::ArraySeparatorOrEnd;
                return true;
            }
            case CSONTokenType::OpenCurlyBrace:
            {
                // We have to add an object to our array
                auto obj = CSONObject(CSONObjectType::Object);
                // Set the parent of this object to our current (array)
                obj.parent      = m_pCurrentObject;
                CSONValue value = obj;
                // Add the object to the array
                m_pCurrentObject->properties.EmplaceBack(value);
                // Set the current object to the object we just added
                m_pCurrentObject = &std::get<CSONObject>(m_pCurrentObject->properties.Last().value);
                // Set the parse mode to start checking for items in the object
                m_parseMode = CSONParseMode::KeyOrEndOfObject;
                return true;
            }
            case CSONTokenType::CloseSquareBrace:
            {
                if (m_parseMode == CSONParseMode::ArrayValueAfterOpen)
                {
                    // Since we just parsed the array open it's ok to find the close bracket (since then it's just an empty array)
                    m_parseMode = CSONParseMode::CommaOrEndOfObject;
                    // We should also start parsing for the parent object again
                    m_pCurrentObject = m_pCurrentObject->parent;
                    return true;
                }
            }
        }

        return ParseError(token, "a valid value");
    }

    bool CSONParser::ParseArraySeparatorOrEnd(const CSONToken& token)
    {
        if (token.type == CSONTokenType::Comma)
        {
            // Separator found. So let's find another value
            m_parseMode = CSONParseMode::ArrayValueAfterComma;
            return true;
        }

        if (token.type == CSONTokenType::CloseSquareBrace)
        {
            // End of the array found. Next token should be a comma (or the end of the object)
            m_parseMode = CSONParseMode::CommaOrEndOfObject;
            // We should also start parsing for the parent object again
            m_pCurrentObject = m_pCurrentObject->parent;
            return true;
        }

        return ParseError(token, "',' or ']'");
    }

    bool CSONParser::ParseEndOfFile(const CSONToken& token)
    {
        if (token.type == CSONTokenType::EndOfFile)
        {
            // We are done
            return true;
        }

        return ParseError(token, "end of file");
    }

    bool CSONParser::ParseError(const CSONToken& token, const char* expected)
    {
        ERROR_LOG("Parsing error on line: {}. Expected: '{}' but found: '{}'.", token.line, expected,
                  m_pInput->SubStr(token.start, token.end + 1));
        return false;
    }

    CSONObject CSONParser::ParseTokens()
    {
        // Initially we always expect an object or an array
        m_parseMode = CSONParseMode::ObjectOrArray;

        // The root node that we should always have
        CSONObject root(CSONObjectType::Object);

        // Get a pointer to our current object (which starts at the root)
        m_pCurrentObject = &root;

        auto token = m_tokens.Pop();
        while (token.type != CSONTokenType::EndOfFile)
        {
            // Skip comments and whitespace
            if (token.type == CSONTokenType::Whitespace || token.type == CSONTokenType::NewLine || token.type == CSONTokenType::Comment)
            {
                token = m_tokens.Pop();
                continue;
            }

            switch (m_parseMode)
            {
                case CSONParseMode::CommaOrEndOfObject:
                    if (!ParseCommaOrEndOfObject(token)) return root;
                    break;
                case CSONParseMode::Colon:
                    if (!ParseColon(token)) return root;
                    break;
                case CSONParseMode::KeyOrEndOfObject:
                    if (!ParseKeyOrEndOfObject(token)) return root;
                    break;
                case CSONParseMode::Value:
                    if (!ParseValue(token)) return root;
                    break;
                case CSONParseMode::ObjectOrArray:
                    if (!ParseArrayOrObject(token)) return root;
                    break;
                case CSONParseMode::ArrayValueAfterOpen:
                case CSONParseMode::ArrayValueAfterComma:
                    if (!ParseArrayValue(token)) return root;
                    break;
                case CSONParseMode::ArraySeparatorOrEnd:
                    if (!ParseArraySeparatorOrEnd(token)) return root;
                    break;
                case CSONParseMode::EndOfFile:
                    if (!ParseEndOfFile(token)) return root;
                    break;
            }

            // Get the next token
            token = m_tokens.Pop();
        }

        return root;
    }

}  // namespace C3D