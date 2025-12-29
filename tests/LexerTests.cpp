#include "TestFramework.h"
#include "../src/lexer/Lexer.h"
#include "../src/utils/Error.h"
#include <string>

using namespace sigma;
using namespace sigma::test;

// ============================================================================
// Basic Token Tests
// ============================================================================

TEST(Lexer_SingleCharTokens) {
    Lexer lexer("(){},:~^");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::LPAREN);
    EXPECT_TRUE(tokens[1].type == TokenType::RPAREN);
    EXPECT_TRUE(tokens[2].type == TokenType::LBRACE);
    EXPECT_TRUE(tokens[3].type == TokenType::RBRACE);
    EXPECT_TRUE(tokens[4].type == TokenType::COMMA);
    EXPECT_TRUE(tokens[5].type == TokenType::COLON);
    EXPECT_TRUE(tokens[6].type == TokenType::BIT_NOT);
    EXPECT_TRUE(tokens[7].type == TokenType::BIT_XOR);
    EXPECT_TRUE(tokens[8].type == TokenType::END_OF_FILE);
    
    return true;
}

TEST(Lexer_BracketTokens) {
    Lexer lexer("[ ] [0] arr[1]");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::LBRACKET);
    EXPECT_TRUE(tokens[1].type == TokenType::RBRACKET);
    EXPECT_TRUE(tokens[2].type == TokenType::LBRACKET);
    EXPECT_TRUE(tokens[3].type == TokenType::NUMBER);
    EXPECT_TRUE(tokens[4].type == TokenType::RBRACKET);
    EXPECT_TRUE(tokens[5].type == TokenType::IDENTIFIER);
    EXPECT_TRUE(tokens[6].type == TokenType::LBRACKET);
    EXPECT_TRUE(tokens[7].type == TokenType::NUMBER);
    EXPECT_TRUE(tokens[8].type == TokenType::RBRACKET);
    
    return true;
}

TEST(Lexer_Operators) {
    Lexer lexer("+ - * / % = == != < <= > >= && || ! & |");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::PLUS);
    EXPECT_TRUE(tokens[1].type == TokenType::MINUS);
    EXPECT_TRUE(tokens[2].type == TokenType::STAR);
    EXPECT_TRUE(tokens[3].type == TokenType::SLASH);
    EXPECT_TRUE(tokens[4].type == TokenType::PERCENT);
    EXPECT_TRUE(tokens[5].type == TokenType::ASSIGN);
    EXPECT_TRUE(tokens[6].type == TokenType::EQ);
    EXPECT_TRUE(tokens[7].type == TokenType::NEQ);
    EXPECT_TRUE(tokens[8].type == TokenType::LT);
    EXPECT_TRUE(tokens[9].type == TokenType::LEQ);
    EXPECT_TRUE(tokens[10].type == TokenType::GT);
    EXPECT_TRUE(tokens[11].type == TokenType::GEQ);
    EXPECT_TRUE(tokens[12].type == TokenType::AND);
    EXPECT_TRUE(tokens[13].type == TokenType::OR);
    EXPECT_TRUE(tokens[14].type == TokenType::NOT);
    EXPECT_TRUE(tokens[15].type == TokenType::BIT_AND);
    EXPECT_TRUE(tokens[16].type == TokenType::BIT_OR);
    
    return true;
}

TEST(Lexer_CompoundAssignment) {
    Lexer lexer("+= -= *= /= %=");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::PLUS_EQ);
    EXPECT_TRUE(tokens[1].type == TokenType::MINUS_EQ);
    EXPECT_TRUE(tokens[2].type == TokenType::STAR_EQ);
    EXPECT_TRUE(tokens[3].type == TokenType::SLASH_EQ);
    EXPECT_TRUE(tokens[4].type == TokenType::PERCENT_EQ);
    
    return true;
}

TEST(Lexer_IncrementDecrement) {
    Lexer lexer("++ --");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::PLUS_PLUS);
    EXPECT_TRUE(tokens[1].type == TokenType::MINUS_MINUS);
    
    return true;
}

TEST(Lexer_BitShift) {
    Lexer lexer("<< >>");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::LSHIFT);
    EXPECT_TRUE(tokens[1].type == TokenType::RSHIFT);
    
    return true;
}

// ============================================================================
// Literal Tests
// ============================================================================

TEST(Lexer_Numbers) {
    Lexer lexer("123 45.67 0 3.14159");
    auto tokens = lexer.scanTokens();
    
    // Integer literals now stored as int64_t
    EXPECT_TRUE(tokens[0].type == TokenType::NUMBER);
    EXPECT_TRUE(std::holds_alternative<int64_t>(tokens[0].literal));
    EXPECT_EQ(std::get<int64_t>(tokens[0].literal), 123);
    
    // Float literals stored as double
    EXPECT_TRUE(tokens[1].type == TokenType::NUMBER);
    EXPECT_TRUE(std::holds_alternative<double>(tokens[1].literal));
    EXPECT_EQ(std::get<double>(tokens[1].literal), 45.67);
    
    // Zero is an integer
    EXPECT_TRUE(tokens[2].type == TokenType::NUMBER);
    EXPECT_TRUE(std::holds_alternative<int64_t>(tokens[2].literal));
    EXPECT_EQ(std::get<int64_t>(tokens[2].literal), 0);
    
    // Float with many decimals
    EXPECT_TRUE(tokens[3].type == TokenType::NUMBER);
    EXPECT_TRUE(std::holds_alternative<double>(tokens[3].literal));
    EXPECT_TRUE(std::abs(std::get<double>(tokens[3].literal) - 3.14159) < 0.0001);
    
    return true;
}

TEST(Lexer_Strings) {
    Lexer lexer("\"hello\" \"world with spaces\" \"\"");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::STRING);
    EXPECT_EQ(std::get<std::string>(tokens[0].literal), "hello");
    
    EXPECT_TRUE(tokens[1].type == TokenType::STRING);
    EXPECT_EQ(std::get<std::string>(tokens[1].literal), "world with spaces");
    
    EXPECT_TRUE(tokens[2].type == TokenType::STRING);
    EXPECT_EQ(std::get<std::string>(tokens[2].literal), "");
    
    return true;
}

TEST(Lexer_InterpolatedStrings) {
    Lexer lexer("\"Hello {name}!\"");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::INTERP_STRING);
    EXPECT_EQ(std::get<std::string>(tokens[0].literal), "Hello {name}!");
    
    return true;
}

// ============================================================================
// Keyword Tests
// ============================================================================

TEST(Lexer_Keywords) {
    Lexer lexer("fr say lowkey midkey highkey goon vibe send ongod cap nah skip mog");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::FR);
    EXPECT_TRUE(tokens[1].type == TokenType::SAY);
    EXPECT_TRUE(tokens[2].type == TokenType::LOWKEY);
    EXPECT_TRUE(tokens[3].type == TokenType::MIDKEY);
    EXPECT_TRUE(tokens[4].type == TokenType::HIGHKEY);
    EXPECT_TRUE(tokens[5].type == TokenType::GOON);
    EXPECT_TRUE(tokens[6].type == TokenType::VIBE);
    EXPECT_TRUE(tokens[7].type == TokenType::SEND);
    EXPECT_TRUE(tokens[8].type == TokenType::ONGOD);
    EXPECT_TRUE(tokens[9].type == TokenType::CAP);
    EXPECT_TRUE(tokens[10].type == TokenType::NAH);
    EXPECT_TRUE(tokens[11].type == TokenType::SKIP);
    EXPECT_TRUE(tokens[12].type == TokenType::MOG);
    
    return true;
}

TEST(Lexer_MoreKeywords) {
    Lexer lexer("edge simp stan ghost yeet caught");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::EDGE);
    EXPECT_TRUE(tokens[1].type == TokenType::SIMP);
    EXPECT_TRUE(tokens[2].type == TokenType::STAN);
    EXPECT_TRUE(tokens[3].type == TokenType::GHOST);
    EXPECT_TRUE(tokens[4].type == TokenType::YEET);
    EXPECT_TRUE(tokens[5].type == TokenType::CAUGHT);
    
    return true;
}

TEST(Lexer_Identifiers) {
    Lexer lexer("myVar _private camelCase UPPER_CASE x123 _");
    auto tokens = lexer.scanTokens();
    
    for (int i = 0; i < 6; i++) {
        EXPECT_TRUE(tokens[i].type == TokenType::IDENTIFIER);
    }
    
    EXPECT_EQ(tokens[0].lexeme, "myVar");
    EXPECT_EQ(tokens[1].lexeme, "_private");
    EXPECT_EQ(tokens[2].lexeme, "camelCase");
    EXPECT_EQ(tokens[3].lexeme, "UPPER_CASE");
    EXPECT_EQ(tokens[4].lexeme, "x123");
    EXPECT_EQ(tokens[5].lexeme, "_");
    
    return true;
}

// ============================================================================
// Comment and Whitespace Tests
// ============================================================================

TEST(Lexer_Comments) {
    Lexer lexer("fr x = 5 # this is a comment\nfr y = 10");
    auto tokens = lexer.scanTokens();
    
    // Comment should be skipped, only fr x = 5 and fr y = 10
    EXPECT_TRUE(tokens[0].type == TokenType::FR);
    EXPECT_TRUE(tokens[1].type == TokenType::IDENTIFIER);
    EXPECT_TRUE(tokens[2].type == TokenType::ASSIGN);
    EXPECT_TRUE(tokens[3].type == TokenType::NUMBER);
    EXPECT_TRUE(tokens[4].type == TokenType::FR);
    
    return true;
}

TEST(Lexer_Whitespace) {
    Lexer lexer("  \t  fr  \t  x  \n  =  \r\n  5  ");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::FR);
    EXPECT_TRUE(tokens[1].type == TokenType::IDENTIFIER);
    EXPECT_TRUE(tokens[2].type == TokenType::ASSIGN);
    EXPECT_TRUE(tokens[3].type == TokenType::NUMBER);
    
    return true;
}

TEST(Lexer_LineTracking) {
    Lexer lexer("fr\nx\n=\n5");
    auto tokens = lexer.scanTokens();
    
    EXPECT_EQ(tokens[0].line, 1);  // fr
    EXPECT_EQ(tokens[1].line, 2);  // x
    EXPECT_EQ(tokens[2].line, 3);  // =
    EXPECT_EQ(tokens[3].line, 4);  // 5
    
    return true;
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST(Lexer_UnterminatedString) {
    ErrorReporter::reset();
    Lexer lexer("\"hello");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(lexer.hasError());
    EXPECT_TRUE(ErrorReporter::hadError());
    
    return true;
}

TEST(Lexer_UnexpectedCharacter) {
    ErrorReporter::reset();
    Lexer lexer("fr x = 5 @ 3");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(lexer.hasError());
    
    return true;
}

// ============================================================================
// Complex Expression Tests
// ============================================================================

TEST(Lexer_ComplexExpression) {
    Lexer lexer("fr result = (a + b) * c / 2");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::FR);
    EXPECT_TRUE(tokens[1].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "result");
    EXPECT_TRUE(tokens[2].type == TokenType::ASSIGN);
    EXPECT_TRUE(tokens[3].type == TokenType::LPAREN);
    EXPECT_TRUE(tokens[4].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[4].lexeme, "a");
    EXPECT_TRUE(tokens[5].type == TokenType::PLUS);
    EXPECT_TRUE(tokens[6].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[6].lexeme, "b");
    EXPECT_TRUE(tokens[7].type == TokenType::RPAREN);
    EXPECT_TRUE(tokens[8].type == TokenType::STAR);
    EXPECT_TRUE(tokens[9].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[9].lexeme, "c");
    EXPECT_TRUE(tokens[10].type == TokenType::SLASH);
    EXPECT_TRUE(tokens[11].type == TokenType::NUMBER);
    EXPECT_TRUE(std::holds_alternative<int64_t>(tokens[11].literal));
    EXPECT_EQ(std::get<int64_t>(tokens[11].literal), 2);
    
    return true;
}

TEST(Lexer_FunctionDefinition) {
    Lexer lexer("vibe add(a, b) { send a + b }");
    auto tokens = lexer.scanTokens();
    
    EXPECT_TRUE(tokens[0].type == TokenType::VIBE);
    EXPECT_TRUE(tokens[1].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[1].lexeme, "add");
    EXPECT_TRUE(tokens[2].type == TokenType::LPAREN);
    EXPECT_TRUE(tokens[3].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[3].lexeme, "a");
    EXPECT_TRUE(tokens[4].type == TokenType::COMMA);
    EXPECT_TRUE(tokens[5].type == TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[5].lexeme, "b");
    EXPECT_TRUE(tokens[6].type == TokenType::RPAREN);
    EXPECT_TRUE(tokens[7].type == TokenType::LBRACE);
    EXPECT_TRUE(tokens[8].type == TokenType::SEND);
    EXPECT_TRUE(tokens[9].type == TokenType::IDENTIFIER);
    EXPECT_TRUE(tokens[10].type == TokenType::PLUS);
    EXPECT_TRUE(tokens[11].type == TokenType::IDENTIFIER);
    EXPECT_TRUE(tokens[12].type == TokenType::RBRACE);
    
    return true;
}
