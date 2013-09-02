#pragma once

#include "ofConstants.h"
#include "ofBaseTypes.h"

// this #define is for use by addon writers, to conditionally support sound objects in addons
// (while allowing backwards compatibility with previous versions of openFrameworks)
#define OF_SOUND_OBJECT

/**
 * ofSoundObject is a node in your dsp chain. It can have one input, 
 * and one output. If it doesn't have an input, it's the beginning
 * of the dsp chain (i.e. an oscillator or a microphone input).
 */
class ofSoundObject: public ofBaseSoundOutput {
public:
	ofSoundObject();
	virtual ~ofSoundObject() {}
	
	/// Connects the output of this ofSoundObject to the input of the parameter ofSoundObject
	ofSoundObject &connectTo(ofSoundObject &soundObject);
    void disconnect();
    void disconnectInput(ofSoundObject * input);	
	
	/// This is the method you implement to process the signal from inputs to outputs.
	virtual void process(ofSoundBuffer &input, ofSoundBuffer &output) {
		// default behaviour is pass-through.
		input.copyTo(output);
	}
	
	/// this pulls the audio through from earlier links in the chain.
	/// you can override this to add more interesting functionality
	/// like signal splitters, sidechains etc.
	virtual void audioOut(ofSoundBuffer &output);
	
	/// this checks the dsp chain to ensure there are no infinite loops
	/// - might want to override this if you make a splitter
	/// returns true if there are no infinite loops.
	virtual bool checkForInfiniteLoops();
    ofSoundObject *getInputObject();
	
protected:
	// this is the previous dsp object in the chain
	// that feeds this one with input.
	ofSoundObject *inputObject;
    ofSoundObject *outputObjectRef;
    virtual void setInput(ofSoundObject *obj);
	
private:
	// ofSoundObjects reference their source, not their destination
	// because it's not needed in a pullthrough audio architecture.
	// this lets that be set under the hood via connectTo()

	// a spare buffer to pass from one sound object to another
	ofSoundBuffer workingBuffer;	
};

/**
 * This class represents input from the sound card in your dsp chain.
 */
class ofSoundInput: public ofBaseSoundInput, public ofSoundObject {
public:
	ofSoundInput();

	// copy audio in to internal buffer
	void audioIn(ofSoundBuffer &input);
	void audioOut(ofSoundBuffer &output);
	
private:
	ofSoundBuffer inputBuffer;

};

/**
 * This class represents the output in your dsp chain.
 */
class ofSoundOutput: public ofSoundObject {};
