#ifndef COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H
#define COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H

#include <functional>
#include <string>
#include <utility>

#include "Cocktail/Diagnostics/DiagnosticKind.h"
#include "llvm/ADT/Any.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"

namespace Cocktail {

enum class DiagnosticLevel : int8_t {
  Note,  // 注意，本身不表示错误，但可能为错误提供上下文。
  Warning,  // 警告，表明程序可能存在问题。
  Error,    // 错误，指示程序无效。
};

// 用于创建诊断。
#define COCKTAIL_DIAGNOSTIC(DiagnosticName, Level, Format, ...) \
  static constexpr auto DiagnosticName =                        \
      ::Cocktail::Internal::DiagnosticBase<__VA_ARGS__>(        \
          ::Cocktail::DiagnosticKind::DiagnosticName,           \
          ::Cocktail::DiagnosticLevel::Level, Format)

/// 用于表示诊断在文件中的位置。
struct DiagnosticLocation {
  // 此诊断所引用的文件或缓冲区的名称。
  std::string file_name;
  // 对错误行的引用。
  llvm::StringRef line;
  // 以1为基础的行号。
  int32_t line_number;
  // 以1为基础的列号。
  int32_t column_number;
};

/// 用于表示一个诊断消息。
struct DiagnosticMessage {
  explicit DiagnosticMessage(
      DiagnosticKind kind, DiagnosticLocation location,
      llvm::StringLiteral format, llvm::SmallVector<llvm::Any> format_args,
      std::function<std::string(const DiagnosticMessage&)> format_fn)
      : kind(kind),
        location(location),
        format(format),
        format_args(std::move(format_args)),
        format_fn(std::move(format_fn)) {}

  // 诊断类型。
  DiagnosticKind kind;
  // 诊断位置。
  DiagnosticLocation location;
  // 诊断的格式字符串。这将与format_args一起传递给format_fn。
  llvm::StringLiteral format;
  // 格式化参数列表。
  llvm::SmallVector<llvm::Any> format_args;
  // 返回格式化字符串。默认情况下使用llvm::formatv。
  std::function<std::string(const DiagnosticMessage&)> format_fn;
};

/// 用于表示一个完整的诊断，包括级别、主消息和附加注释。
struct Diagnostic {
  // 诊断等级。
  DiagnosticLevel level;
  // 主要错误或警告。
  DiagnosticMessage message;
  // 向诊断添加上下文或补充信息的说明。
  llvm::SmallVector<DiagnosticMessage> notes;
};

/// 接收发出的诊断信息。
class DiagnosticConsumer {
 public:
  virtual ~DiagnosticConsumer() = default;

  // 用于处理一个诊断（错误、警告或注释）。
  //
  // 诊断对象目前是在栈上分配的，因此它们的生命周期是`HandleDiagnostic`函数的生命周期。
  // `SortingDiagnosticConsumer`类需要更长的诊断对象生命周期，直到所有诊断信息已生成。
  // 目前没有持久地存储诊断，因为在集成开发环境（IDE）中，通常是立即打印并丢弃诊断。
  virtual auto HandleDiagnostic(Diagnostic diagnostic) -> void = 0;

  // 用于刷新任何缓冲的输入。
  virtual auto Flush() -> void {}
};

/// 可以将某个位置的某种表示形式转换为诊断位置的接口。
template <typename LocationT>
class DiagnosticLocationTranslator {
 public:
  virtual ~DiagnosticLocationTranslator() = default;

  [[nodiscard]] virtual auto GetLocation(LocationT loc)
      -> DiagnosticLocation = 0;
};

namespace Internal {

template <typename... Args>
struct DiagnosticBase {
  explicit constexpr DiagnosticBase(DiagnosticKind kind, DiagnosticLevel level,
                                    llvm::StringLiteral format)
      : Kind(kind), Level(level), Format(format) {}

  // 使用诊断参数调用formatv。
  auto FormatFn(const DiagnosticMessage& message) const -> std::string {
    return FormatFnImpl(message, std::make_index_sequence<sizeof...(Args)>());
  };

  // 诊断类型。
  DiagnosticKind Kind;
  // 诊断级别。
  DiagnosticLevel Level;
  // llvm::formatv的诊断格式。
  llvm::StringLiteral Format;

 private:
  // 处理llvm::Any转换为formatv的Args类型。
  template <std::size_t... N>
  inline auto FormatFnImpl(const DiagnosticMessage& message,
                           std::index_sequence<N...> /*indices*/) const
      -> std::string {
    assert(message.format_args.size() == sizeof...(Args));
    return llvm::formatv(message.format.data(),
                         llvm::any_cast<Args>(message.format_args[N])...);
  }
};

// 禁用基于' args '的类型推导。
template <typename Arg>
using NoTypeDeduction = std::common_type_t<Arg>;

}  // namespace Internal

template <typename LocationT, typename AnnotateFn>
class DiagnosticAnnotationScope;

/// 管理诊断报告的创建，测试诊断是否启用，以及报告的收集。
///
/// 这个类由位置类型参数化，允许不同的诊断客户端以最方便的形式提供位置信息，
/// 例如在词法分析时缓冲区内的位置，在解析时的tokenw位置，或在类型检查时的解析树
/// 节点位置，并允许单元测试与任何具体的位置表示解耦。
template <typename LocationT>
class DiagnosticEmitter {
 public:
  // 一个建造者模式类型，提供接口来构造更复杂的诊断。
  class DiagnosticBuilder {
   public:
    DiagnosticBuilder(DiagnosticBuilder&&) = default;
    auto operator=(DiagnosticBuilder&&) -> DiagnosticBuilder& = default;

    // 向正在构建的主诊断添加附加的注释。
    template <typename... Args>
    auto Note(LocationT location,
              const Internal::DiagnosticBase<Args...>& diagnostic_base,
              Internal::NoTypeDeduction<Args>... args) -> DiagnosticBuilder& {
      COCKTAIL_CHECK(diagnostic_base.Level == DiagnosticLevel::Note)
          << static_cast<int>(diagnostic_base.Level);
      diagnostic_.notes.push_back(MakeMessage(
          emitter_, location, diagnostic_base, {llvm::Any(args)...}));
      return *this;
    }

    // 用于发出构建的诊断和其附加的注释。
    template <typename... Args>
    auto Emit() -> void {
      for (auto* annotator_ : emitter_->annotators_) {
        annotator_->Annotate(*this);
      }
      emitter_->consumer_->HandleDiagnostic(std::move(diagnostic_));
    }

   private:
    friend class DiagnosticEmitter<LocationT>;

    template <typename... Args>
    explicit DiagnosticBuilder(
        DiagnosticEmitter<LocationT>* emitter, LocationT location,
        const Internal::DiagnosticBase<Args...>& diagnostic_base,
        llvm::SmallVector<llvm::Any> args)
        : emitter_(emitter),
          diagnostic_(
              {.level = diagnostic_base.Level,
               .message = MakeMessage(emitter, location, diagnostic_base,
                                      std::move(args))}) {
      COCKTAIL_CHECK(diagnostic_base.Level != DiagnosticLevel::Note);
    }

    template <typename... Args>
    static auto MakeMessage(
        DiagnosticEmitter<LocationT>* emitter, LocationT location,
        const Internal::DiagnosticBase<Args...>& diagnostic_base,
        llvm::SmallVector<llvm::Any> args) -> DiagnosticMessage {
      return DiagnosticMessage(
          diagnostic_base.Kind, emitter->translator_->GetLocation(location),
          diagnostic_base.Format, std::move(args),
          [&diagnostic_base](const DiagnosticMessage& message) -> std::string {
            return diagnostic_base.FormatFn(message);
          });
    }

    DiagnosticEmitter<LocationT>* emitter_;
    Diagnostic diagnostic_;
  };

  explicit DiagnosticEmitter(
      DiagnosticLocationTranslator<LocationT>& translator,
      DiagnosticConsumer& consumer)
      : translator_(&translator), consumer_(&consumer) {}
  ~DiagnosticEmitter() = default;

  // 发出一个错误。
  template <typename... Args>
  auto Emit(LocationT location,
            const Internal::DiagnosticBase<Args...>& diagnostic_base,
            Internal::NoTypeDeduction<Args>... args) -> void {
    DiagnosticBuilder(this, location, diagnostic_base, {llvm::Any(args)...})
        .Emit();
  }

  // 返回一个`DiagnosticBuilder`对象，允许你构建一个更复杂的诊断。
  template <typename... Args>
  auto Build(LocationT location,
             const Internal::DiagnosticBase<Args...>& diagnostic_base,
             Internal::NoTypeDeduction<Args>... args) -> DiagnosticBuilder {
    return DiagnosticBuilder(this, location, diagnostic_base,
                             {llvm::Any(args)...});
  }

 private:
  // 在其中执行诊断注释的作用域的基类，例如添加带有上下文信息的注释。
  class DiagnosticAnnotationScopeBase {
   public:
    virtual auto Annotate(DiagnosticBuilder& builder) -> void = 0;

    DiagnosticAnnotationScopeBase(const DiagnosticAnnotationScopeBase&) =
        delete;
    auto operator=(const DiagnosticAnnotationScopeBase&)
        -> DiagnosticAnnotationScopeBase& = delete;

   protected:
    DiagnosticAnnotationScopeBase(DiagnosticEmitter* emitter)
        : emitter_(emitter) {
      emitter_->annotators_.push_back(this);
    }
    ~DiagnosticAnnotationScopeBase() {
      COCKTAIL_CHECK(emitter_->annotators_.back() == this);
      emitter_->annotators_.pop_back();
    }

   private:
    DiagnosticEmitter* emitter_;
  };

  template <typename LocT, typename AnnotateFn>
  friend class DiagnosticAnnotationScope;

  DiagnosticLocationTranslator<LocationT>* translator_;  // 位置转换器
  DiagnosticConsumer* consumer_;                         // 诊断消费者
  llvm::SmallVector<DiagnosticAnnotationScopeBase*> annotators_;  // 注释器
};

/// 将诊断信息以人类可读的形式输出到一个流（stream）中，主要用于在控制台或文件中显示诊断信息。
/// 这个类是责任链中的最终消费者，它的任务是终端输出而不是将诊断信息传递给其他消费者。
class StreamDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit StreamDiagnosticConsumer(llvm::raw_ostream& stream)
      : stream_(&stream) {}

  auto HandleDiagnostic(Diagnostic diagnostic) -> void override {
    Print(diagnostic.message);
    for (const auto& note : diagnostic.notes) {
      Print(note);
    }
  }
  auto Print(const DiagnosticMessage& message) -> void {
    *stream_ << message.location.file_name;
    if (message.location.line_number > 0) {
      *stream_ << ":" << message.location.line_number;
      if (message.location.column_number > 0) {
        *stream_ << ":" << message.location.column_number;
      }
    }
    *stream_ << ": " << message.format_fn(message) << "\n";
    if (message.location.column_number > 0) {
      *stream_ << message.location.line << "\n";
      stream_->indent(message.location.column_number - 1);
      *stream_ << "^\n";
    }
  }

 private:
  llvm::raw_ostream* stream_;
};

inline auto ConsoleDiagnosticConsumer() -> DiagnosticConsumer& {
  static auto* consumer = new StreamDiagnosticConsumer(llvm::errs());
  return *consumer;
}

/// 用于跟踪是否产生了任何错误。
class ErrorTrackingDiagnosticConsumer : public DiagnosticConsumer {
 public:
  explicit ErrorTrackingDiagnosticConsumer(DiagnosticConsumer& next_consumer)
      : next_consumer_(&next_consumer) {}

  // 处理一个诊断，并检查是否有错误。
  auto HandleDiagnostic(Diagnostic diagnostic) -> void override {
    seen_error_ |= diagnostic.level == DiagnosticLevel::Error;
    next_consumer_->HandleDiagnostic(std::move(diagnostic));
  }

  // 重置错误跟踪状态。
  auto Reset() -> void { seen_error_ = false; }

  // 返回自上次重置以来是否看到了错误。
  auto seen_error() const -> bool { return seen_error_; }

 private:
  DiagnosticConsumer* next_consumer_;
  bool seen_error_ = false;
};

/// 这是一个 RAII对象，用于标记在其作用范围内应以某种方式注释任何生成的诊断。
///
/// 这个对象被赋予一个函数 annotate，该函数将在通过给定的发射器发出任何诊断时
/// 与 DiagnosticBuilder& builder 一起调用。该函数可以通过调用 builder.Note
/// 来添加注释以注释诊断。
template <typename LocationT, typename AnnotateFn>
class DiagnosticAnnotationScope
    : private DiagnosticEmitter<LocationT>::DiagnosticAnnotationScopeBase {
  using Base =
      typename DiagnosticEmitter<LocationT>::DiagnosticAnnotationScopeBase;

 public:
  DiagnosticAnnotationScope(DiagnosticEmitter<LocationT>* emitter,
                            AnnotateFn annotate)
      : Base(emitter), annotate_(annotate) {}

 private:
  // 调用注释函数以注释诊断。
  auto Annotate(
      typename DiagnosticEmitter<LocationT>::DiagnosticBuilder& builder)
      -> void override {
    annotate_(builder);
  }

  AnnotateFn annotate_;
};

// 类类型自动推导。
template <typename LocationT, typename AnnotateFn>
DiagnosticAnnotationScope(DiagnosticEmitter<LocationT>* emitter,
                          AnnotateFn annotate)
    -> DiagnosticAnnotationScope<LocationT, AnnotateFn>;

}  // namespace Cocktail

#endif  // COCKTAIL_DIAGNOSTICS_DIAGNOSTIC_EMITTER_H