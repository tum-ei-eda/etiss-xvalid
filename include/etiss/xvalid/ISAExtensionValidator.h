#ifndef ETISS_XVALID_ISAEXTENSIONVALIDATOR_H_
#define ETISS_XVALID_ISAEXTENSIONVALIDATOR_H_

#include "etiss/Plugin.h"

namespace etiss
{
namespace plugin
{

class ISAExtensionValidator : public etiss::TranslationPlugin
{
  public:
    void initInstrSet(etiss::instr::ModedInstructionSet &) const override;
    void finalizeInstrSet(etiss::instr::ModedInstructionSet &) const override;
    void initCodeBlock(etiss::CodeBlock &) const override;
    void finalizeCodeBlock(etiss::CodeBlock &) const override;
    void *getPluginHandle() override;
    std::string _getPluginName() const override;
};

} // namespace plugin
} // namespace etiss

#endif
