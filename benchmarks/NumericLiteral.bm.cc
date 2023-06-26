#include "Cocktail/Lexer/NumericLiteral.h"

#include <benchmark/benchmark.h>

#include "Cocktail/Common/Check.h"
#include "Cocktail/Diagnostics/NullDiagnostics.h"

namespace {

using namespace Cocktail;

static void BM_Lex_Float(benchmark::State& state) {
  for (auto _ : state) {
    COCKTAIL_CHECK(LexedNumericLiteral::Lex("0.000001"));
  }
}

static void BM_Lex_Integer(benchmark::State& state) {
  for (auto _ : state) {
    COCKTAIL_CHECK(LexedNumericLiteral::Lex("1_234_567_890"));
  }
}

static void BM_ComputeValue_Float(benchmark::State& state) {
  auto val = LexedNumericLiteral::Lex("0.000001");
  COCKTAIL_CHECK(val);
  auto emitter = NullDiagnosticEmitter<const char*>();
  for (auto _ : state) {
    val->ComputeValue(emitter);
  }
}

static void BM_ComputeValue_Integer(benchmark::State& state) {
  auto val = LexedNumericLiteral::Lex("1_234_567_890");
  auto emitter = NullDiagnosticEmitter<const char*>();
  COCKTAIL_CHECK(val);
  for (auto _ : state) {
    val->ComputeValue(emitter);
  }
}

BENCHMARK(BM_Lex_Float);
BENCHMARK(BM_Lex_Integer);
BENCHMARK(BM_ComputeValue_Float);
BENCHMARK(BM_ComputeValue_Integer);

}  // namespace