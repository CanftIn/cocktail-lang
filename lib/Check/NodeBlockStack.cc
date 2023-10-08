#include "Cocktail/Check/NodeBlockStack.h"

#include "Cocktail/Common/VLog.h"
#include "Cocktail/SemIR/Node.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringExtras.h"

namespace Cocktail::Check {

auto NodeBlockStack::Push(SemIR::NodeBlockId id) -> void {
  COCKTAIL_VLOG() << name_ << " Push " << size_ << "\n";
  COCKTAIL_CHECK(size_ < (1 << 20))
      << "Excessive stack size: likely infinite loop";
  if (size_ == static_cast<int>(stack_.size())) {
    stack_.emplace_back();
  }
  stack_[size_].Reset(id);
  ++size_;
}

auto NodeBlockStack::PeekOrAdd(int depth) -> SemIR::NodeBlockId {
  COCKTAIL_CHECK(size() > depth) << "no such block";
  int index = size() - depth - 1;
  auto& slot = stack_[index];
  if (!slot.id.is_valid()) {
    slot.id = semantics_ir_->AddNodeBlockId();
  }
  return slot.id;
}

auto NodeBlockStack::Pop() -> SemIR::NodeBlockId {
  COCKTAIL_CHECK(!empty()) << "no current block";
  --size_;
  auto& back = stack_[size_];

  // Finalize the block.
  if (!back.content.empty() && back.id != SemIR::NodeBlockId::Unreachable) {
    if (back.id.is_valid()) {
      semantics_ir_->SetNodeBlock(back.id, back.content);
    } else {
      back.id = semantics_ir_->AddNodeBlock(back.content);
    }
  }

  COCKTAIL_VLOG() << name_ << " Pop " << size_ << ": " << back.id << "\n";
  if (!back.id.is_valid()) {
    return SemIR::NodeBlockId::Empty;
  }
  return back.id;
}

auto NodeBlockStack::PopAndDiscard() -> void {
  COCKTAIL_CHECK(!empty()) << "no current block";
  --size_;
  COCKTAIL_VLOG() << name_ << " PopAndDiscard " << size_ << "\n";
}

auto NodeBlockStack::PrintForStackDump(llvm::raw_ostream& output) const
    -> void {
  output << name_ << ":\n";
  for (const auto& item : llvm::enumerate(stack_)) {
    auto i = item.index();
    auto entry = item.value();
    output << "\t" << i << ".\t" << entry.id << "\t{";
    llvm::ListSeparator sep;
    for (auto id : entry.content) {
      output << sep << id;
    }
    output << "}\n";
  }
}

}  // namespace Cocktail::Check