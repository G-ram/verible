// Copyright 2017-2019 The Verible Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "common/lexer/lexer_test_util.h"

#include <initializer_list>
#include <iterator>
#include <vector>

#include "gtest/gtest.h"
#include "absl/strings/match.h"
#include "absl/strings/string_view.h"
#include "common/text/constants.h"
#include "common/text/token_info.h"
#include "common/util/logging.h"

namespace verible {
namespace {

TEST(ShowCodeStreamableTest, ContainsCode) {
  std::ostringstream stream;
  constexpr char code[] = "abc.xyz";
  stream << ShowCode{code};
  EXPECT_TRUE(absl::StrContains(stream.str(), code));
}

// Modeled after Lexer base class in lexer.h.
class FakeLexer {
 protected:
  explicit FakeLexer() {}

  void SetTokensData(std::initializer_list<TokenInfo> tokens) {
    tokens_ = tokens;
    tokens_iter_ = tokens_.begin();
  }

 public:
  const TokenInfo& DoNextToken() {
    CHECK(tokens_iter_ != tokens_.cend());
    return *tokens_iter_++;
  }

 private:
  std::vector<TokenInfo> tokens_;
  std::vector<TokenInfo>::const_iterator tokens_iter_;
};

// For use with TestDriverTokenInfos only
class TestDriverTokenInfosFakeLexer : public FakeLexer {
 public:
  explicit TestDriverTokenInfosFakeLexer(absl::string_view code)
      : joined_text_(code) {
    static const std::initializer_list<TokenInfo> driver_data = {
        // TokenInfo: enum, text
        {3, joined_text_.substr(0, 3)},
        {5, joined_text_.substr(3, 2)},
        {TK_EOF, joined_text_.substr(5, 0)},
    };
    SetTokensData(driver_data);
  }

 private:
  absl::string_view joined_text_;
};

TEST(SynthesizedLexerTestDataTest, TestDriverTokenInfos) {
  const std::initializer_list<SynthesizedLexerTestData> test_data = {
      {
          {3, "bar"}, {5, "++"},
          // omit the EOF token
      },
  };
  TestLexer<TestDriverTokenInfosFakeLexer>(test_data);
}

// For use with TestDriverDontCares only
class TestDriverDontCaresFakeLexer : public FakeLexer {
 public:
  explicit TestDriverDontCaresFakeLexer(absl::string_view code)
      : joined_text_(code) {
    static const std::initializer_list<TokenInfo> driver_data = {
        // TokenInfo: enum, left, text
        {3, joined_text_.substr(0, 3)},
        // The next three tokens span ".:;"
        {'.', joined_text_.substr(3, 1)},
        {':', joined_text_.substr(4, 1)},
        {';', joined_text_.substr(5, 1)},
        {5, joined_text_.substr(6, 2)},
        {TK_EOF, joined_text_.substr(8, 0)},
    };
    SetTokensData(driver_data);
  }

 private:
  absl::string_view joined_text_;
};

TEST(SynthesizedLexerTestDataTest, TestDriverDontCares) {
  const std::initializer_list<SynthesizedLexerTestData> test_data = {
      {
          {3, "BAR"},
          // Don't care about these tokens' enums,
          // or how this excerpt is tokenized:
          ".:;",
          {5, "--"},
          // omit the EOF token
      },
  };
  TestLexer<TestDriverDontCaresFakeLexer>(test_data);
}

}  // namespace
}  // namespace verible
