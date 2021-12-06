/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider){
    using namespace juce;
    auto bounds= Rectangle<float>(x,y,width,height);
    g.setColour(Colour(6u,14u,127u));
    g.fillEllipse(bounds);

    g.setColour(Colour(60u, 100u,127u));
    g.drawEllipse(bounds, 1.f);

    if(auto*rswl=dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();
        Path p;
        Rectangle<float> r;
        r.setLeft(center.getX()-2);
        r.setRight(center.getX()+2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY()-rswl->getTextHeight()*1.5);
        p.addRoundedRectangle(r,2.f);
        jassert(rotaryStartAngle<rotaryEndAngle);
        auto sliderAngRad=jmap(sliderPosProportional,0.f,1.f,rotaryStartAngle,rotaryEndAngle);
        p.applyTransform(AffineTransform().rotated(sliderAngRad,center.getX(),center.getY()));
        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text=rswl->getDisplayString();
        auto strWidth=g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth+4, rswl->getTextHeight()+2);
        r.setCentre(bounds.getCentre());
        g.setColour(Colours::black);
        g.fillRect(r);

        g.setColour(Colours::white);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred,1);
    }

}
void RotarySliderWithLabels::paint(juce::Graphics &g){
    using namespace juce;
    auto startAngle=degreesToRadians(180.f+45.f);
    auto endAngle=degreesToRadians(180.f-45.f+360);
    auto range=getRange();
    auto sliderBounds=getSliderBounds();

   // g.setColour(Colours::black);
   // g.drawRect(getLocalBounds());
    //g.setColour(Colours::yellow);
    //g.drawRect(sliderBounds);

    getLookAndFeel().drawRotarySlider(g,sliderBounds.getX(),sliderBounds.getY(),sliderBounds.getWidth(),
        sliderBounds.getHeight(),jmap(getValue(), range.getStart(), range.getEnd(),0.0,1.0), startAngle, endAngle, *this );

    auto center= sliderBounds.toFloat().getCentre();
    auto radius=sliderBounds.getWidth() *.5f;



    g.setColour(Colour(120u,80u,120u));
    g.setFont(getTextHeight());
    auto numChoices=labels.size();
    for(int i=0;i<numChoices;++i)
    {
        auto pos=labels[i].pos;
        jassert(0.f<=pos);
        jassert(pos<=1.f);
        auto ang=jmap(pos,0.f,1.f,startAngle,endAngle);

        auto c= center.getPointOnCircumference(radius+getTextHeight() *.5f+1, ang);
        Rectangle<float> r;
        auto str=labels[i].label;
        r.setSize(g.getCurrentFont().getStringWidth(str),getTextHeight());
        r.setCentre(c);
        r.setY(r.getY()+getTextHeight());
        g.drawFittedText(str,r.toNearestInt(),juce::Justification::centred,1);
    }
}
juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds= getLocalBounds();

    auto size=juce::jmin(bounds.getWidth(),bounds.getHeight());

    size-=getTextHeight()*2;

    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(),0);
    r.setY(2);

    return r;
}
juce::String RotarySliderWithLabels::getDisplayString() const
{
    if(auto* choiceParam=dynamic_cast<juce::AudioParameterChoice*>(param))
            return choiceParam->getCurrentChoiceName();
    juce::String str;
    bool addK=false;
    if(auto* floatParam=dynamic_cast<juce::AudioParameterFloat*>(param))
    {
        float val= getValue();
        if(val>999.f)
        {
            val/=1000.f;
            addK = true;
        }
        str= juce::String(val, (addK? 2 : 0));
    }
    if(suffix.isNotEmpty())
    {
        str<<" ";
        if(addK)
            str<<"k";
        str<<suffix;
    }
    return str;
}
ResponseCurveComponent::ResponseCurveComponent(FiveBandEQAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for(auto param: params){
        param->addListener(this);
    }

    startTimerHz(60);
}
ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for(auto param: params){
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if( parametersChanged.compareAndSetBool(false,true))
    {
        //update monochain
        auto chainSettings=getChainSettings(audioProcessor.apvts);
        auto peak1Coefficients=makePeak1Filter(chainSettings, audioProcessor.getSampleRate());
        auto peak2Coefficients=makePeak2Filter(chainSettings, audioProcessor.getSampleRate());
        auto peak3Coefficients=makePeak3Filter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients,peak1Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients,peak2Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak3>().coefficients,peak3Coefficients);
        auto lowCutCoefficients= makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients= makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(),lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(),highCutCoefficients, chainSettings.highCutSlope);
        //signal new draw of response curve
        repaint();
    }
}
void ResponseCurveComponent::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (Colours::black);
    g.drawImage(background, getLocalBounds().toFloat());
    auto responseArea = getAnalysisArea();

    auto w=responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    auto& peak3 = monoChain.get<ChainPositions::Peak3>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();

    auto sampleRate= audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for(int i=0; i<w; ++i){
        double mag=1.f;
        auto freq = mapToLog10(double(i)/double(w), 20.0,20000.0);

        if(! monoChain.isBypassed<ChainPositions::Peak1>() )
            mag*=peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! monoChain.isBypassed<ChainPositions::Peak2>() )
            mag*=peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! monoChain.isBypassed<ChainPositions::Peak3>() )
            mag*=peak3.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(! lowcut.isBypassed<0>() )
            mag*=lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! lowcut.isBypassed<1>() )
            mag*=lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! lowcut.isBypassed<2>() )
            mag*=lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! lowcut.isBypassed<3>() )
            mag*=lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if(! highcut.isBypassed<0>() )
            mag*=highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! highcut.isBypassed<1>() )
            mag*=highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! highcut.isBypassed<2>() )
            mag*=highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if(! highcut.isBypassed<3>() )
            mag*=highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i]=Decibels::gainToDecibels(mag);
    }

    Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax=responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

    for( size_t i=1; i<mags.size(); ++i)
    {
        responseCurve.lineTo(responseArea.getX()+i, map(mags[i]));
    }

    g.setColour(Colours::white);
    g.drawRoundedRectangle(getRenderArea().toFloat(), 4.f, 1.f);
    g.setColour(Colours::blue);
    g.strokePath(responseCurve, PathStrokeType(2.f));
}


void ResponseCurveComponent::resized()
{
    using namespace juce;
    background = Image(Image::PixelFormat::RGB, getWidth(), getHeight(), true);
    Graphics g(background);

    Array<float> freqs
    {
        20,30,40,50,100,
        200,300,400,500,1000,
        2000,3000,4000,5000,10000,
        20000
    };

    auto renderArea=getAnalysisArea();
    auto left=renderArea.getX();
    auto right=renderArea.getRight();
    auto top=renderArea.getY();
    auto bottom=renderArea.getBottom();
    auto width=renderArea.getWidth();

    Array<float> xs;
    for(auto f : freqs )
    {
        auto normX=mapFromLog10(f, 20.f, 20000.f);
        xs.add(left+width*normX);
    }

    g.setColour(Colours::grey);
    for(auto x: xs)
    {
      //  auto normX = mapFromLog10(f, 20.f, 20000.f);
     //   g.drawVerticalLine(getWidth() * normX, 0.f, getHeight());
        g.drawVerticalLine(x,top,bottom);
    }

    Array<float> gain
    {
        -24,-12,0,12,24
    };

    for(auto gDb: gain)
    {
        auto y=jmap(gDb,-24.f,24.f,float(bottom),float(top));
     //   g.drawHorizontalLine(y,0,getWidth());
        g.setColour(gDb == 0.f ? Colour(120u,80u,120u): Colours::white);
        g.drawHorizontalLine(y,left,right);
    }
 //   g.drawRect(getAnalysisArea());
    g.setColour(Colours::white);
    const int fontHeight=10;
    g.setFont(fontHeight);
    for(int i=0;i<freqs.size(); ++i)
    {
        auto f=freqs[i];
        auto x=xs[i];

        bool addK=false;
        String str;
        if(f>999.f)
        {
            addK=true;
            f/=1000.f;
        }
        str<<f;
        if(addK)
            str<<"k";
        str<<"Hz";

        auto textWidth=g.getCurrentFont().getStringWidth(str);
        Rectangle<int> r;
        r.setSize(textWidth, fontHeight);
        r.setCentre(x,0);
        r.setY(1);

        g.drawFittedText(str,r,juce::Justification::centred,1);

        for(auto gDb : gain)
        {
            auto y=jmap(gDb,-24.f,24.f,float(bottom),float(top));
            String str;
            if(gDb>0)
                str<<"+";
            str<<gDb;

            auto textWidth=g.getCurrentFont().getStringWidth(str);

            Rectangle<int> r;
            r.setSize(textWidth,fontHeight);
            r.setX(getWidth()-textWidth);
            r.setCentre(r.getCentreX(),y);

            g.setColour(gDb == 0.f ? Colour(120u,80u,120u): Colours::white);

            g.drawFittedText(str, r, juce::Justification::centred, 1);

            str.clear();
            str<< (gDb-24.f);

            r.setX(1);
            textWidth=g.getCurrentFont().getStringWidth(str);
            r.setSize(textWidth, fontHeight);
            g.setColour(Colours::white);
            g.drawFittedText(str, r, juce::Justification::centred, 1);
        }
    }
}

juce::Rectangle<int> ResponseCurveComponent::getRenderArea()
{
    auto bounds =getLocalBounds();
    bounds.removeFromTop(12);
    bounds.removeFromBottom(2);
    bounds.removeFromLeft(20);
    bounds.removeFromRight(20);
    return bounds;
}
juce::Rectangle<int> ResponseCurveComponent::getAnalysisArea()
{
    auto bounds = getRenderArea();
    bounds.removeFromTop(4);
    bounds.removeFromBottom(4);
    return bounds;
}
//==============================================================================
FiveBandEQAudioProcessorEditor::FiveBandEQAudioProcessorEditor (FiveBandEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
peak1FreqSlider(*audioProcessor.apvts.getParameter("Peak1 Freq"), "Hz"),
peak2FreqSlider(*audioProcessor.apvts.getParameter("Peak2 Freq"), "Hz"),
peak3FreqSlider(*audioProcessor.apvts.getParameter("Peak3 Freq"), "Hz"),
peak1GainSlider(*audioProcessor.apvts.getParameter("Peak1 Gain"), "dB"),
peak2GainSlider(*audioProcessor.apvts.getParameter("Peak2 Gain"), "dB"),
peak3GainSlider(*audioProcessor.apvts.getParameter("Peak3 Gain"), "dB"),
peak1QualitySlider(*audioProcessor.apvts.getParameter("Peak1 Quality"), "dB/Oct"),
peak2QualitySlider(*audioProcessor.apvts.getParameter("Peak2 Quality"), "dB/Oct"),
peak3QualitySlider(*audioProcessor.apvts.getParameter("Peak3 Quality"), "dB/Oct"),
lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Freq"), "Hz"),
highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Freq"), "Hz"),
lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "dB/Oct"),
responseCurveComponent(audioProcessor),
peak1FreqSliderAttachment(audioProcessor.apvts, "Peak1 Freq", peak1FreqSlider),
peak1GainSliderAttachment(audioProcessor.apvts, "Peak1 Gain", peak1GainSlider),
peak1QualitySliderAttachment(audioProcessor.apvts, "Peak1 Quality", peak1QualitySlider),
peak2FreqSliderAttachment(audioProcessor.apvts, "Peak2 Freq", peak2FreqSlider),
peak2GainSliderAttachment(audioProcessor.apvts, "Peak2 Gain", peak2GainSlider),
peak2QualitySliderAttachment(audioProcessor.apvts, "Peak2 Quality", peak2QualitySlider),
peak3FreqSliderAttachment(audioProcessor.apvts, "Peak3 Freq", peak3FreqSlider),
peak3GainSliderAttachment(audioProcessor.apvts, "Peak3 Gain", peak3GainSlider),
peak3QualitySliderAttachment(audioProcessor.apvts, "Peak3 Quality", peak3QualitySlider),
lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Freq", lowCutFreqSlider),
highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Freq", highCutFreqSlider),
lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCutSlope", lowCutSlopeSlider),
highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCutSlope", highCutSlopeSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    peak1FreqSlider.labels.add({0.f,"20Hz"});
    peak1FreqSlider.labels.add({1.f,"20kHz"});
    peak1GainSlider.labels.add({0.f, "-24dB"});
    peak1GainSlider.labels.add({1.f,"24dB"});
    peak1QualitySlider.labels.add({0.f,"0.1"});
    peak1QualitySlider.labels.add({1.f, "10.0"});
    peak2FreqSlider.labels.add({0.f,"20Hz"});
    peak2FreqSlider.labels.add({1.f,"20kHz"});
    peak2GainSlider.labels.add({0.f, "-24dB"});
    peak2GainSlider.labels.add({1.f,"24dB"});
    peak2QualitySlider.labels.add({0.f,"0.1"});
    peak2QualitySlider.labels.add({1.f, "10.0"});
    peak3FreqSlider.labels.add({0.f,"20Hz"});
    peak3FreqSlider.labels.add({1.f,"20kHz"});
    peak3GainSlider.labels.add({0.f, "-24dB"});
    peak3GainSlider.labels.add({1.f,"24dB"});
    peak3QualitySlider.labels.add({0.f,"0.1"});
    peak3QualitySlider.labels.add({1.f, "10.0"});
    lowCutFreqSlider.labels.add({0.f,"20Hz"});
    lowCutFreqSlider.labels.add({1.f,"20kHz"});
    highCutFreqSlider.labels.add({0.f,"20Hz"});
    highCutFreqSlider.labels.add({1.f,"20kHz"});
    lowCutSlopeSlider.labels.add({0.0f, "12"});
    lowCutSlopeSlider.labels.add({1.f, "48"});   
    highCutSlopeSlider.labels.add({0.0f, "12"});
    highCutSlopeSlider.labels.add({1.f, "48"}); 

    for( auto* comp : getComps () )
    {
        addAndMakeVisible(comp);
    }
    
    setSize (1000, 800);
}

FiveBandEQAudioProcessorEditor::~FiveBandEQAudioProcessorEditor()
{
    
}

//==============================================================================




void FiveBandEQAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * .33);
    responseCurveComponent.setBounds(responseArea);
    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth()*.20);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * .25);

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight()*.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight()*.5));
    highCutSlopeSlider.setBounds(highCutArea);

    auto peakArea = bounds;

    auto peak1Area = peakArea.removeFromLeft(peakArea.getWidth()*.33);
    auto peak3Area= peakArea.removeFromRight(peakArea.getWidth()*.5);

    peak2FreqSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * .33));
    peak2GainSlider.setBounds(peakArea.removeFromTop(peakArea.getHeight() * .33));
    peak2QualitySlider.setBounds(peakArea);


    peak1FreqSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .33));
    peak1GainSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * .33));
    peak1QualitySlider.setBounds(peak1Area);

    
    peak3FreqSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * .33));
    peak3GainSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * .33));
    peak3QualitySlider.setBounds(peak3Area);


}










std::vector<juce::Component*> FiveBandEQAudioProcessorEditor::getComps()
{
    return
    {
        &peak1FreqSlider,
        &peak2FreqSlider,
        &peak3FreqSlider,
        &peak1GainSlider,
        &peak2GainSlider,
        &peak3GainSlider,
        &peak1QualitySlider,
        &peak2QualitySlider,
        &peak3QualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent
    };
}