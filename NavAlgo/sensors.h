/* ************************************************************************** */
// Initialize and read from sensors
/* ************************************************************************** */

// structs for storing the sensor data
#ifndef date_h
#define date_h
typedef struct date {
  int year;
  unsigned char month;
  unsigned char day;
  unsigned char hour;
  unsigned char minute;
  unsigned char seconds;
} date_t;

typedef struct dataStructure {
  float boat_direction; //Boat direction w.r.t North (IMU)
  float sailAngleBoat; //Sail angle for use of finding wind wrt boat
  float tailAngleBoat; //Tail angle for use of finding wind wrt boat
  float pitch; // (IMU)
  float roll; // (IMU)
  float wind_dir; // Wind direction w.r.t North
  float wind_speed;
  double x;
  double y;
  double lat;
  double longi;
  date_t dateTime; // Current date and time, of type date_t
} data_t;
#endif

extern data_t* sensorData; //Defines the boat's state, in type data_t

void initSensors(void);

void readIMU(void);