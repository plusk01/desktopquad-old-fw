#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>

#include "board.h"
#include "mixer.h"
#include "param.h"
#include "mode.h"
#include "rc.h"

static float prescaled_outputs[8];
float _outputs[8];
command_t _command;

static mixer_t quadcopter_plus_mixing =
{
  {M, M, M, M, NONE, NONE, NONE, NONE}, // output_type

  { 1.0f,  1.0f,  1.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // F Mix
  { 0.0f, -1.0f,  1.0f,  0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // X Mix
  {-1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Y Mix
  {-1.0f,  1.0f,  1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f}  // Z Mix
};


static mixer_t quadcopter_x_mixing =
{
  {M, M, M, M, NONE, NONE, NONE, NONE}, // output_type

  { 1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f}, // F Mix
  {-1.0f,-1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f}, // X Mix
  {-1.0f, 1.0f,-1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f}, // Y Mix
  {-1.0f, 1.0f, 1.0f,-1.0f,  0.0f, 0.0f, 0.0f, 0.0f}  // Z Mix
};

static mixer_t quadcopter_h_mixing =
{
  {M, M, M, M, NONE, NONE, NONE, NONE}, // output_type

  { 1.0f, 1.0f, 1.0f, 1.0f,  0.0f, 0.0f, 0.0f, 0.0f}, // F Mix
  {-1057, -943, 1057,  943,  0.0f, 0.0f, 0.0f, 0.0f}, // X Mix
  {-1005,  995,-1005,  995,  0.0f, 0.0f, 0.0f, 0.0f}, // Y Mix
  {-1.0f, 1.0f, 1.0f,-1.0f,  0.0f, 0.0f, 0.0f, 0.0f}  // Z Mix
};

static mixer_t fixedwing_mixing =
{
  {S, S, M, S, NONE, NONE, NONE, NONE},

  { 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // F Mix
  { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // X Mix
  { 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Y Mix
  { 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f}  // Z Mix
};

static mixer_t tricopter_mixing =
{
  {M, M, M, S, NONE, NONE, NONE, NONE},

  { 1.0f,     1.0f,   1.0f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // F Mix
  {-1.0f,     1.0f,   0.0f,   0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // X Mix
  {-0.667f,  -0.667f, 1.333f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}, // Y Mix
  { 0.0f,     0.0f,   0.0f,   1.0f, 0.0f, 0.0f, 0.0f, 0.0f}  // Z Mix
};

static mixer_t Y6_mixing =
{
  {M, M, M, M, M, M, NONE, NONE},
  { 1.0f,   1.0f,    1.0f,    1.0f,    1.0f,    1.0f,   0.0f, 0.0f}, // F Mix
  {-1.0f,  -1.0f,    0.0f,    0.0f,    1.0f,    1.0f,   0.0f, 0.0f}, // X Mix
  { 0.667f, 0.667f, -1.333f, -1.333f,  0.667f,  0.667f, 0.0f, 0.0f}, // Y Mix
  {-1.0f,   1.0f,   -1.0f,    1.0f,   -1.0f,    1.0f,   0.0f, 0.0f}  // Z Mix
};

static mixer_t X8_mixing =
{
  {M, M, M, M, M, M, M, M},
  { 1.0f,   1.0f,    1.0f,    1.0f,    1.0f,    1.0f,   1.0f,  1.0f}, // F Mix
  {-1.0f,   1.0f,    1.0f,   -1.0f,    1.0f,   -1.0f,  -1.0f,  1.0f}, // X Mix
  { 1.0f,   1.0f,   -1.0f,   -1.0f,    1.0f,    1.0f,  -1.0f, -1.0f}, // Y Mix
  { 1.0f,  -1.0f,    1.0f,   -1.0f,    1.0f,   -1.0f,   1.0f, -1.0f}  // Z Mix
};

static mixer_t* mixer_to_use;

static mixer_t *array_of_mixers[NUM_MIXERS] =
{
  &quadcopter_plus_mixing,
  &quadcopter_x_mixing,
  &quadcopter_h_mixing,
  &tricopter_mixing,
  &Y6_mixing,
  &X8_mixing,
  &fixedwing_mixing
};



void init_mixing()
{
  // We need a better way to choosing the mixer
  mixer_to_use = array_of_mixers[get_param_int(PARAM_MIXER)];

  for (int8_t i=0; i<8; i++)
  {
    _outputs[i] = 0.0f;
    prescaled_outputs[i] = 0.0f;
  }
  _command.F = 0;
  _command.x = 0;
  _command.y = 0;
  _command.z = 0;
}

void init_PWM()
{
  bool useCPPM = false;
  if (get_param_int(PARAM_RC_TYPE) == 1)
  {
    useCPPM = true;
  }
  int16_t motor_refresh_rate = get_param_int(PARAM_MOTOR_PWM_SEND_RATE);
  int16_t off_pwm = get_param_int(PARAM_MOTOR_MIN_PWM);
  pwm_init(useCPPM, motor_refresh_rate, off_pwm);
}


void write_motor(uint8_t index, float value)
{
  if (_armed_state == ARMED)
  {
    if (value > 1.0)
    {
      value = 1.0;
    }
    else if (value < get_param_float(PARAM_MOTOR_IDLE_THROTTLE) && get_param_int(PARAM_SPIN_MOTORS_WHEN_ARMED))
    {
      value = get_param_float(PARAM_MOTOR_IDLE_THROTTLE);
    }
    else if (value < 0.0)
    {
      value = 0.0;
    }
  }
  else
  {
    value = 0.0;
  }
  _outputs[index] = value;
  int32_t pwm_us = value * (get_param_int(PARAM_MOTOR_MAX_PWM) - get_param_int(PARAM_MOTOR_MIN_PWM)) + get_param_int(PARAM_MOTOR_MIN_PWM);
  pwm_write(index, pwm_us);
}


void write_servo(uint8_t index, float value)
{
  if (value > 1.0)
  {
    value = 1.0;
  }
  else if (value < -1.0)
  {
    value = -1.0;
  }
  _outputs[index] = value;
  pwm_write(index, _outputs[index] * 500 + 1500);
}


void mix_output()
{
  float max_output = 1.0f;

  // Reverse Fixedwing channels just before mixing if we need to
  if (get_param_int(PARAM_FIXED_WING))
  {
    _command.x *= get_param_int(PARAM_AILERON_REVERSE) ? -1 : 1;
    _command.y *= get_param_int(PARAM_ELEVATOR_REVERSE) ? -1 : 1;
    _command.z *= get_param_int(PARAM_RUDDER_REVERSE) ? -1 : 1;
  }

  for (int8_t i=0; i<8; i++)
  {
    if (mixer_to_use->output_type[i] != NONE)
    {
      // Matrix multiply to mix outputs
      prescaled_outputs[i] = (_command.F*mixer_to_use->F[i] + _command.x*mixer_to_use->x[i] +
                              _command.y*mixer_to_use->y[i] + _command.z*mixer_to_use->z[i]);

      // Save off the largest control output if it is greater than 1.0 for future scaling
      if (prescaled_outputs[i] > max_output)
      {
        max_output = prescaled_outputs[i];
      }
    }
  }

  // saturate outputs to maintain controllability even during aggressive maneuvers
  float scale_factor = 1.0;
  if (max_output > 1.0)
  {
    scale_factor = 1.0/max_output;
  }

  // Add in GPIO inputs from Onboard Computer
  for (int8_t i=0; i<8; i++)
  {
    // Write output to motors
    if (mixer_to_use->output_type[i] == S)
    {
      write_servo(i, prescaled_outputs[i]);
    }
    else if (mixer_to_use->output_type[i] == M)
    {
      // scale all motor outputs by scale factor (this is usually 1.0, unless we saturated)
      prescaled_outputs[i] *= scale_factor;
      write_motor(i, prescaled_outputs[i]);
    }
  }
}


#ifdef __cplusplus
}
#endif
