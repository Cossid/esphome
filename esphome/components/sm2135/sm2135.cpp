#include "sm2135.h"
#include "esphome/core/log.h"

// Tnx to the work of https://github.com/arendst (Tasmota) for making the initial version of the driver

namespace esphome {
namespace sm2135 {

static const char *const TAG = "sm2135";

static const uint8_t SM2135_ADDR_MC = 0xC0;  // Max current register
static const uint8_t SM2135_RGB = 0x00;  // RGB channel
static const uint8_t SM2135_CW = 0x80;   // CW channel (Chip default)

static const uint8_t SM2135_DELAY_SHORT = 2;
static const uint8_t SM2135_DELAY_LONG = 4;

void SM2135::setup() {
  ESP_LOGCONFIG(TAG, "Setting up SM2135OutputComponent...");
  this->data_pin_->setup();
  this->data_pin_->digital_write(true);
  this->clock_pin_->setup();
  this->clock_pin_->digital_write(true);
  this->pwm_amounts_.resize(5, 0);
}

void SM2135::dump_config() {
  ESP_LOGCONFIG(TAG, "SM2135:");
  LOG_PIN("  Data Pin: ", this->data_pin_);
  LOG_PIN("  Clock Pin: ", this->clock_pin_);
  ESP_LOGCONFIG(TAG, "  CW Current: %dmA", 10 + (this->cw_current_ * 5));
  ESP_LOGCONFIG(TAG, "  RGB Current: %dmA", 10 + (this->rgb_current_ * 5));
}

void SM2135::loop() {
  if (!this->update_)
    return;

  uint8_t data[8];
  data[0] = SM2135_ADDR_MC;
  data[1] = current_mask_;

  if (this->separate_modes_) {
    if (this->update_channel_ == 3 || this->update_channel_ == 4) {
      // No color so must be Cold/Warm

      data[2] = SM2135_CW;
      data[3] = 0;
      data[4] = 0;
      data[5] = 0;
      data[6] = this->pwm_amounts_[3];
      data[7] = this->pwm_amounts_[4];
      this->write_buffer_(data, 8);
    } else {
      // Color

      data[2] = SM2135_RGB;
      data[3] = this->pwm_amounts_[0];
      data[4] = this->pwm_amounts_[1];
      data[5] = this->pwm_amounts_[2];
      this->write_buffer_(data, 6);
    }
  } else {
    data[2] = SM2135_RGB;
    data[3] = this->pwm_amounts_[0];
    data[4] = this->pwm_amounts_[1];
    data[5] = this->pwm_amounts_[2];
    data[6] = this->pwm_amounts_[3];
    data[7] = this->pwm_amounts_[4];
    this->write_buffer_(data, 8);
  }

  this->update_ = false;
}

void SM2135::set_channel_value_(uint8_t channel, uint8_t value) {
  if (this->pwm_amounts_[channel] != value) {
    this->update_ = true;
    this->update_channel_ = channel;
  }
  this->pwm_amounts_[channel] = value;
}

void SM2135::write_bit_(bool value) {
  this->data_pin_->digital_write(value);
  this->clock_pin_->digital_write(true);
  delayMicroseconds(SM2135_DELAY_LONG);
  this->clock_pin_->digital_write(false);
}

void SM2135::write_byte_(uint8_t data) {
  for (uint8_t mask = 0x80; mask; mask >>= 1) {
    this->write_bit_(data & mask);
  }

  // ack bit
  this->data_pin_->digital_write(true);
  this->clock_pin_->digital_write(true);
  delayMicroseconds(SM2135_DELAY_SHORT);
  this->clock_pin_->digital_write(false);
  delayMicroseconds(SM2135_DELAY_SHORT);
  this->data_pin_->digital_write(false);
}

void SM2135::write_buffer_(uint8_t *buffer, uint8_t size) {
  this->data_pin_->digital_write(false);
  delayMicroseconds(SM2135_DELAY_LONG);
  this->clock_pin_->digital_write(false);
  this->data_pin_->digital_write(false);

  for (uint32_t i = 0; i < size; i++) {
    this->write_byte_(buffer[i]);
  }

  this->data_pin_->digital_write(false);
  delayMicroseconds(SM2135_DELAY_LONG);
  this->clock_pin_->digital_write(true);
  delayMicroseconds(SM2135_DELAY_LONG);
  this->data_pin_->digital_write(true);
  delayMicroseconds(SM2135_DELAY_LONG);
}

}  // namespace sm2135
}  // namespace esphome
