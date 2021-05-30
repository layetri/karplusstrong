//
// Created by Daniël Kamp on 17/02/2021.
//

#include "Header/Buffer.h"
#include <iostream>

Buffer::Buffer(int length) {
  data = new int16_t [length];
  size = length;
  position = 0;
}

Buffer::~Buffer() {
  delete[] data;
}

int Buffer::getSize() {
  return size;
}

int Buffer::getPosition() {
  return position;
}

int16_t Buffer::getSample(int sample_position) {
//  int pos = ((sample_position > size || (sample_position * -1) > size) * sample_position % size) +
//      ((sample_position < size && (sample_position * -1) < size) * sample_position);

  if(sample_position < 0) {
    return this->operator[](size + sample_position);
  } else {
    return this->operator[](sample_position);
  }
}

int16_t Buffer::getCurrentSample() {
  return this->operator[](position);
}

void Buffer::tick() {
  if(position < size) {
    position++;
  } else {
    position -= size;
  }
}

void Buffer::write(int16_t sample) {
  data[position] = sample;
}

void Buffer::writeAhead(int16_t sample, int places) {
  data[position + places] = sample;
}