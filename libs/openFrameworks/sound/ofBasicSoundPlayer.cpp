/*
 * ofBasicSoundPlayer.cpp
 *
 *  Created on: 25/07/2012
 *      Author: arturo
 */

#include "ofBasicSoundPlayer.h"
#include "ofSoundUtils.h"
#include <float.h>

int ofBasicSoundPlayer::maxSoundsTotal=128;
int ofBasicSoundPlayer::maxSoundsPerPlayer=16;


ofBasicSoundPlayer::ofBasicSoundPlayer() {
	volume = 1;
	streaming = false;
	isPlaying = false;
	multiplay = false;
	bIsLoaded = false;
	positions.resize(1,0);
	loop = false;
	speed = 1;
	relativeSpeed.resize(1,1);
	volumesLeft.resize(1,1);
	volumesRight.resize(1,1);
	pan = 0;
	maxSounds = maxSoundsPerPlayer;
}

ofBasicSoundPlayer::~ofBasicSoundPlayer() {
	unloadSound();
}



bool ofBasicSoundPlayer::loadSound(string fileName, bool _stream){
	
	ofLogNotice() << "loading " << fileName;

	ofLogNotice() << "opening file ";
	bIsLoaded = soundFile.open(fileName);
	if(!bIsLoaded) return false;

	streaming = _stream;
	if ( streaming ){
		speed = 1;
	}
	
	if(!streaming){
		ofLogNotice() << "reading whole file ";
		soundFile.readTo(buffer);
	}// else, buffer will be resized on audioOutBuffersChanged

	return true;
}

void ofBasicSoundPlayer::audioOutBuffersChanged(int nFrames, int nChannels, int sampleRate){
	if(streaming){
		ofLogNotice() << "resizing buffer ";
		buffer.resize(nFrames*nChannels,0);
	}
	playerNumFrames = nFrames;
	playerNumChannels = nChannels;
	playerSampleRate = sampleRate;
}

void ofBasicSoundPlayer::unloadSound(){
	soundFile.close();
	buffer.clear();
	isPlaying = false;
	bIsLoaded = false;
	positions.resize(1,0);
}

void ofBasicSoundPlayer::play(){
	int pos=0;
	float relSpeed = speed*(double(soundFile.getSampleRate())/double(playerSampleRate));
	float left,right;
	ofStereoVolumes(volume,pan,left,right);

	if (multiplay) {
		if(maxSounds>(int)positions.size()){
			positions.push_back(pos);
			relativeSpeed.push_back(relSpeed);
			volumesLeft.push_back(left);
			volumesRight.push_back(right);
		}
	}else{
		if (streaming) {
			soundFile.seekTo(pos);
		}
		positions.back() = pos;
		relativeSpeed.back() = relSpeed;
		volumesLeft.back() = left;
		volumesRight.back() = right;
	}
	isPlaying = true;
}

void ofBasicSoundPlayer::stop(){
	isPlaying = false;

	if (streaming){
		soundFile.seekTo(0);
	}

}


void ofBasicSoundPlayer::setVolume(float vol){
	volume = vol;
	ofStereoVolumes(volume,pan,volumesLeft.back(),volumesRight.back());
}

void ofBasicSoundPlayer::setPan(float _pan){
	pan = _pan;
	ofStereoVolumes(volume,pan,volumesLeft.back(),volumesRight.back());
}

void ofBasicSoundPlayer::setSpeed(float spd){
	if ( streaming && fabsf(spd-1.0f)<FLT_EPSILON ){
		ofLogWarning("ofBasicSoundPlayer") << "setting speed is not supported on streaming sounds";
		return;
	}
	speed = spd;
	relativeSpeed.back() = speed*(double(soundFile.getSampleRate())/double(playerSampleRate));
}

void ofBasicSoundPlayer::setPaused(bool bP){
	isPlaying = false;
}

void ofBasicSoundPlayer::setLoop(bool bLp){
	loop = bLp;
}

void ofBasicSoundPlayer::setMultiPlay(bool bMp){
	multiplay = bMp;
	if(!multiplay){
		positions.resize(1);
		relativeSpeed.resize(1);
		volumesLeft.resize(1);
		volumesRight.resize(1);
	}
}

void ofBasicSoundPlayer::setPosition(float pct){
	positions.back() = pct*buffer.getNumFrames();
}

void ofBasicSoundPlayer::setPositionMS(int ms){
	setPosition(float(ms)/float(buffer.getDurationMS()));
}


float ofBasicSoundPlayer::getPosition(){
	return float(positions.back())/float(buffer.getNumFrames());
}

int ofBasicSoundPlayer::getPositionMS(){
	return float(positions.back())*1000./buffer.getSampleRate();
}

bool ofBasicSoundPlayer::getIsPlaying(){
	return isPlaying;
}

float ofBasicSoundPlayer::getSpeed(){
	return speed;
}

float ofBasicSoundPlayer::getPan(){
	return pan;
}

bool ofBasicSoundPlayer::isLoaded(){
	return bIsLoaded;
}

float ofBasicSoundPlayer::getVolume(){
	return volume;
}

unsigned long ofBasicSoundPlayer::getDurationMS(){
	return buffer.getDurationMS();
}

void ofBasicSoundPlayer::updatePositions(int nFrames){
	for(int i=0;i<(int)positions.size();i++){
		// update positions
		positions[i] += nFrames*relativeSpeed[i];
		if(loop){
			positions[i] %= buffer.getNumFrames();
		}else{
			positions[i] = ofClamp(positions[i],0,buffer.getNumFrames());
			// finished?
			if(positions[i]==buffer.getNumFrames()){
				// yes: remove multiplay instances
				if(positions.size()>1){
					positions.erase(positions.begin()+i);
					relativeSpeed.erase(relativeSpeed.begin()+i);
					volumesLeft.erase(volumesLeft.begin()+i);
					volumesRight.erase(volumesRight.begin()+i);
					i--;
				}
			}
		}
	}
	// finished?
	if(!loop && positions.size()==1 && positions[0]==buffer.getNumFrames()){
		isPlaying = false;
	}
}

void ofBasicSoundPlayer::audioOut(ofSoundBuffer& outputBuffer){
	
	int nFrames = outputBuffer.getNumFrames();
	int nChannels = outputBuffer.getNumChannels();
	
	if(isPlaying){
		if(streaming){
			int samplesRead = soundFile.readTo(buffer,nFrames);
			if ( samplesRead==0 ){
				isPlaying=false;
				soundFile.seekTo(0);
			}
			else{
				buffer.stereoPan(volumesLeft.back(),volumesRight.back());
				newBufferE.notify(this,buffer);
				buffer.copyTo(outputBuffer);
			}
		}else{
			for(int i=0;i<(int)positions.size();i++){
				//assert( resampledBuffer.getNumFrames() == bufferSize*relativeSpeed[i] );
				if(abs(relativeSpeed[i] - 1)<FLT_EPSILON){
					buffer.copyTo(resampledBuffer,nFrames,nChannels,positions[i],loop);
				}else{
					buffer.resampleTo(resampledBuffer,positions[i],nFrames,relativeSpeed[i],loop);
				}
				resampledBuffer.stereoPan(volumesLeft[i],volumesRight[i]);
				newBufferE.notify(this,resampledBuffer);
				resampledBuffer.addTo(outputBuffer,0,loop);
			}
			updatePositions(nFrames);
		}
	}
}

ofSoundBuffer & ofBasicSoundPlayer::getCurrentBuffer(){
	if(streaming){
		return buffer;
	}else{
		return resampledBuffer;
	}
}

void ofBasicSoundPlayer::setMaxSoundsTotal(int max){
	maxSoundsTotal = max;
}

void ofBasicSoundPlayer::setMaxSoundsPerPlayer(int max){
	maxSoundsPerPlayer = max;
}

void ofBasicSoundPlayer::setMaxSounds(int max){
	maxSounds = max;
}
