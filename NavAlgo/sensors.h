/* ************************************************************************** */
// Initialize and read from sensors
/* ************************************************************************** */

// structs for storing the sensor data
#ifndef sensor_h
#define sensor_h

typedef struct dataStructure {
  float boat_direction; //Boat direction w.r.t North (IMU)
  float sailAngleBoat; //Sail angle wrt boat for use of finding wind wrt boat
  float tailAngleBoat; //Tail angle wrt boat for use of finding wind wrt boat
  float pitch; // (IMU)
  float roll; // (IMU)
  float wind_dir; // Wind direction w.r.t North
  volatile float wind_speed;
  double x;
  double y;
  int fix;
  double lat;
  double longi;
  int msec;
  int sec;
  int min;
  int hour;
} data_t;

extern data_t* sensorData; //Defines the boat's state, in type data_t

void initSensors(void);

void convertLLtoXY(void);

void readIMU(void);

void readAnemometer(void);

float readLIDAR(void);

long msTime(void);

#endif