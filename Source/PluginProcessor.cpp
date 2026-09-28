#include "PluginProcessor.h"
#include "PluginEditor.h"

MinimalChainAudioProcessor::MinimalChainAudioProcessor():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)),apvts(*this,nullptr,"PARAMETERS",createParameterLayout()){}
juce::AudioProcessorValueTreeState::ParameterLayout MinimalChainAudioProcessor::createParameterLayout(){
using F=juce::AudioParameterFloat; std::vector<std::unique_ptr<juce::RangedAudioParameter>> p;
p.push_back(std::make_unique<F>("delayTime","Delay Time",juce::NormalisableRange<float>(1,2000,.1f,.35f),350));
p.push_back(std::make_unique<F>("delayFeedback","Delay Feedback",0.f,.95f,.35f));
p.push_back(std::make_unique<F>("delayMix","Delay Mix",0.f,1.f,.25f));
p.push_back(std::make_unique<F>("lpCutoff","Low Pass Cutoff",juce::NormalisableRange<float>(20,20000,1,.25f),12000));
p.push_back(std::make_unique<F>("lpRes","Low Pass Resonance",.1f,1.f,.707f));
p.push_back(std::make_unique<F>("hpCutoff","High Pass Cutoff",juce::NormalisableRange<float>(20,5000,1,.3f),40));
p.push_back(std::make_unique<F>("hpRes","High Pass Resonance",.1f,1.f,.707f));
p.push_back(std::make_unique<F>("distTone","Distortion Tone",0.f,1.f,.5f));
p.push_back(std::make_unique<F>("distAmount","Distortion Amount",0.f,1.f,.15f));
p.push_back(std::make_unique<F>("distMix","Distortion Mix",0.f,1.f,.35f));
p.push_back(std::make_unique<F>("compInput","Compressor Input",0.f,1.f,.5f));
p.push_back(std::make_unique<F>("compPeak","Peak Reduction",0.f,1.f,.25f));
p.push_back(std::make_unique<F>("output","Output",juce::NormalisableRange<float>(-24,12,.01f),0));
return {p.begin(),p.end();}
void MinimalChainAudioProcessor::prepareToPlay(double s,int block){sr=s;delay.setMaximumDelayInSamples((int)(s*2));delay.reset();lp.reset();hp.reset();juce::dsp::ProcessSpec sp{s,(juce::uint32)block,2};lp.prepare(sp);hp.prepare(sp);lp.setType(juce::dsp::StateVariableTPTFilterType::lowpass);hp.setType(juce::dsp::StateVariableTPTFilterType::highpass);}
void MinimalChainAudioProcessor::releaseResources(){delay.reset();}
bool MinimalChainAudioProcessor::isBusesLayoutSupported(const BusesLayout& l)const{return l.getMainInputChannelSet()==l.getMainOutputChannelSet()&&(l.getMainInputChannelSet()==juce::AudioChannelSet::mono()||l.getMainInputChannelSet()==juce::AudioChannelSet::stereo());}
void MinimalChainAudioProcessor::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&){
juce::ScopedNoDenormals nd; int ch=b.getNumChannels(),n=b.getNumSamples();
auto v=[this](const char* id){return apvts.getRawParameterValue(id)->load();};
int ds=juce::jlimit(1,(int)(sr*1.99), (int)(v("delayTime")*.001*sr)); float fb=v("delayFeedback"),mix=v("delayMix");
for(int i=0;i<n;++i)for(int c=0;c<ch;++c){float dry=b.getSample(c,i),wet=delay.popSample(c,(float)ds);delay.pushSample(c,dry+wet*fb);b.setSample(c,i,dry*(1-mix)+wet*mix);}
lp.setCutoffFrequency(v("lpCutoff"));lp.setResonance(v("lpRes"));hp.setCutoffFrequency(v("hpCutoff"));hp.setResonance(v("hpRes"));
juce::dsp::AudioBlock<float> block(b);lp.process(juce::dsp::ProcessContextReplacing<float>(block));hp.process(juce::dsp::ProcessContextReplacing<float>(block));
float tone=v("distTone"),amt=v("distAmount"),dm=v("distMix");
for(int c=0;c<ch;++c){auto*d=b.getWritePointer(c);for(int i=0;i<n;++i){float dry=d[i],wet=std::tanh(dry*(1+15*amt))*(.65f+.35f*tone);d[i]=dry*(1-dm)+wet*dm;}}
float ig=juce::Decibels::decibelsToGain(juce::jmap(v("compInput"),0.f,1.f,-12.f,18.f)),target=v("compPeak")*18.f;
float a=std::exp(-1.f/(.01f*(float)sr)),r=std::exp(-1.f/(.35f*(float)sr));
for(int i=0;i<n;++i){float det=0;for(int c=0;c<ch;++c)det=juce::jmax(det,std::abs(b.getSample(c,i)*ig));float db=juce::Decibels::gainToDecibels(det+1e-9f),gr=juce::jmax(0.f,juce::jmin(target,db));env=gr>env?a*env+(1-a)*gr:r*env+(1-r)*gr;float g=juce::Decibels::decibelsToGain(-env*.75f);for(int c=0;c<ch;++c)b.setSample(c,i,b.getSample(c,i)*ig*g);}
b.applyGain(juce::Decibels::decibelsToGain(v("output")));}
void MinimalChainAudioProcessor::getStateInformation(juce::MemoryBlock& d){if(auto x=apvts.copyState().createXml())copyXmlToBinary(*x,d);}
void MinimalChainAudioProcessor::setStateInformation(const void*d,int s){if(auto x=getXmlFromBinary(d,s))if(x->hasTagName(apvts.state.getType()))apvts.replaceState(juce::ValueTree::fromXml(*x));}
juce::AudioProcessorEditor* MinimalChainAudioProcessor::createEditor(){return new MinimalChainAudioProcessorEditor(*this);}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new MinimalChainAudioProcessor();}
