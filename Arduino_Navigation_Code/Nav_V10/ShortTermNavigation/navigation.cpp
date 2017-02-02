#include <Arduino.h>
#include <math.h>
#include <Servo.h>
#include "sensors.h"
#include "print.h"
#include "navigation.h"

//Define Servo types for Sail and Tail
Servo tailServo;
Servo sailServo;

/*----------Navigation Variables----------*/
int wpNum; //the current waypoint's number in the wayPoints array
int numWP; //total number of waypoints on current course
float detectionradius = 10; //how far away the boat marks a waypoint "reached"
coord_t wayPoints[maxPossibleWaypoints]; //the array containing the waypoints
float normr; //normal distance to the waypoint
float r[2]; //r[0]: Longitude difference b/w current position and waypoint,
            //r[1]: Lattitude difference b/w current position and waypoint
float w[2]; //w[0]: Cosine of the wind direction w.r.t North,
            //w[1]: Sine of the wind direction w.r.t North
float sailAngle;
float tailAngle;
float angleofattack;
float optpolartop;
float optpolarbot;

/*---------Distance to Center Line------*/
// latitude is y, longitude is x
float max_distance=100;
coord_t center_start={1,2};
coord_t center_end={10,20};

float slope=(center_end.latitude - center_start.latitude)/(center_end.longitude - center_start.longitude);
float intercept= center_start.latitude - slope * center_start.longitude;

// takes a coordinate and returns the distance from the coordinate to the center line
float center_distance(coord_t position){
  float top= fabs(slope*position.longitude+position.latitude+intercept);
  float bot= sqrtf(slope*slope + 1);
  return top/bot;

}

/*Servo setup
* "Attaches" servos to defined pins*/
void initServos(void) {
  tailServo.attach(tailServoPin);
  sailServo.attach(sailServoPin);
}

/*Navigation algorithm setup.
* Sets curretn waypoint number and total number of waypoints to 0*/
void initNavigation(void) {
  wpNum = 0;
  numWP = 0;
}

/*----------Stored Coordinates----------*/
//Coordinates in and around the Engineering Quad, Cornell university
coord_t olin = {42.445457, -76.484322}; //Olin Hall
coord_t hollister = {42.444259, -76.484435}; //Hollister Hall
coord_t outsideDuffield = {42.444254, -76.482660}; //Outside West entrance to Duffield Hall, atop the stairs
coord_t outsideThurston = {42.444228, -76.483666}; //In front of Thurston Hall
coord_t engQuadX = {42.444612, -76.483492}; //Center of Engineering Quad
coord_t bottomStairs = {42.444240, -76.483258}; //Bottom of stairs, outside West entrance to Duffield Hall
coord_t northBottomStairs = {42.444735, -76.483275}; //Bottom of stairs, to the left of North entrance to Duffield Hall
coord_t engQuadRight = {42.4444792, -76.483244}; //Middle of the right sector, looking North, of the engineering quad
coord_t sundial = {42.4448630, -76.4835293}; //Sundial on the engineering quad
coord_t northQuadStair= {42.444751, -76.483267}; //bottom of the north stairs
coord_t bottomRightIntersection= {42.444242, -76.483247}; //path intersection bottom right of quad
coord_t thurstonWestEntrance={42.444245, -76.484068}; //west entrance to thurston on the path

//Coordinates in and around the Cornell Sailing Center
coord_t lakeOut = {42.469386,-76.504690}; //Out in the lake, to the left of the Cornell Sailing Center
coord_t lakeOut12 = {42.469847,-76.504522}; //Out in the lake, to the left of the Cornell Sailing Center
coord_t lakeOut2 = {42.469065,-76.506674}; //Out in the lake, to the right of the Cornell Sailing Center
coord_t lakeOut3 = {42.470894,-76.504712}; //Out in the lake, to the right of the Cornell Sailing Center but further North
coord_t shore = {42.469717,-76.503341}; //Far end of the docks, to the left of the Cornell Sailing Center
coord_t shore2 = {42.470862,-76.503950}; //Beach, to the right of the Cornell Sailing Center
coord_t acrossDock = {42.465702, -76.524625}; //Across the lake, when looking from the far edge of the dock to the right of the Cornell Sailing Center
coord_t acrossBeach = {42.467918, -76.525419}; //Across the lake, when looking from the beach to the left of the Cornell Sailing Center
coord_t across_low_dock= {42.468887, -76.504546}; //Across the lake from the low dock
coord_t across_low_dock_test= {42.468923, -76.503655}; //Across the lake from the low dock, halfway to the main point
coord_t low_dock={42.468951,-76.502941}; //Right at the lower dock
coord_t high_dock={42.469552,-76.503353}; //High dock launch point

/*Sets waypoints for navigation
* by creating the wayPoints array*/
void setWaypoints(void) {

  //Make the waypoint array
  numWP = 2;
  wpNum = 0;

  //Set way points to desired coordinates.
  //Assignmment must be of the type coord_t.
  wayPoints[0] = sundial;
  wayPoints[1] = outsideThurston;
}


/*Returns angle (with respect to North) between two global coordinates.*/
float angleToTarget(float lat1, float long1, float lat2, float long2){
  lat1=lat1 * M_PI/180;
  lat2=lat2 * M_PI/180;
  float dLong=(long2-long1) * M_PI/180;
  float y = sin(dLong) * cos(lat2);
  float x = cos(lat1)*sin(lat2) - sin(lat1)*cos(lat2)*cos(dLong);
  float brng = atan2(y, x) * 180/ M_PI;
  if (brng<0){
    brng+=360;
  }
  return brng;
}

/*Returns great circle distance (in meters) between two global coordinates.*/
double havDist(coord_t  first, coord_t second) {
  double x = first.longitude;
  double y = first.latitude;

  double x1 = second.longitude;
  double y1 = second.latitude;

  const double conversion = M_PI / 180;// term to convert from degrees to radians
  const double r = 6371.0;//radius of earth in km
  x = x * conversion;// convert x to radians
  y = y * conversion;// convert y to radians
  x1 = x1 * conversion;
  y1 = y1 * conversion;

  double half1 = (y-y1) / 2;
  double half2 = (x-x1) / 2;

  double part1 = sin(half1) * sin(half1) + cos(y) * cos(y1) * sin(half2) * sin(half2);
  double part2 = sqrt(part1);
  double distance = 2 * r * asin(part2);// distance is in km due to units of earth's radius

  distance = (distance*1000);

  return distance;
}

/*------------------------------------------*/
/*----------Tailvane-angle setters----------*/
/*------------------------------------------*/

// orientation is 1 for right and 0 for left
// right is negative offset, left is positive
// never go upright when facing left
// facing right, angle is above in the sector: w-offset
//   w-(|w+opttop - boatdir|)
float upRight(float b, float w) {
  float offset = fabsf(w+optpolartop-b);
  tailAngle=w-offset;
  // sailAngle=tailAngle+angleofattack;
  return tailAngle;
}

float rightTarget(float b, float w){
	// sailAngle=sensorData.windDir+angleofattack;
	tailAngle=w;
  return tailAngle;
}

float leftTarget(float b, float w){
  // sailAngle=sensorData.windDir-angleofattack;
  tailAngle=w;
  return tailAngle;
}

// facing left, angle is above in the sector: w+offset
//   w+(|w-opttop-boatdir|)
float upLeft(float b, float w){
  float offset = fabsf(w-optpolartop-b);
  tailAngle=w+offset;
  return tailAngle;
  // sailAngle=tailAngle-angleofattack;
}

// facing left, angle is below in the sector: w+offset
//   w+(|w+180-optboat-boatdir|)
float downLeft(float b, float w){
  float offset = fabsf(w+180-optpolarbot-b);
  tailAngle=w+offset;
  return tailAngle;
  // sailAngle=tailAngle-angleofattack;
}
// facing right, angle is below in the sector: w-offset
//   w-(|w+180+optbot-boatdir|)
float downRight(float b, float w){
  float offset = fabsf(w+180+optpolarbot-b);
  tailAngle=w-offset;
  return tailAngle;
  // sailAngle=tailAngle+angleofattack;
}

/*------------------------------------------*/

// converts an angle to a 0-360 range
float convertto360(float angle){
  angle=(float)((int)angle%360);
  angle=angle+360;
  angle=(float)((int)angle%360);
  return angle;
}

/*----------------------------*/

/*----------------------------------------*/
/*----------NAVIGATION ALGORITHM----------*/
/*----------------------------------------*/

/*Uses sensorData.windDir, sensorData.boatDir to set sailAngle and tailAngle.
 * sailAngle and tailAngle are set in terms of servo command numbers, but are first
 * calculated in terms of angle w.r.t the boat direction*/
void nShort(void) {

  printLocationData();
//Unit testing setters
  //sensorData.lati = 42.4441782290;
  //sensorData.lati=outsideThurston.latitude;
  //sensorData.longi=outsideThurston.longitude;
  //sensorData.longi = -76.4834140241;
  sensorData.windDir = 0;
  //sensorData.boatDir = 180;

  //find the normal distance to the waypoint
  r[0] = wayPoints[wpNum].longitude - sensorData.longi;
  r[1] = wayPoints[wpNum].latitude - sensorData.lati;
  w[0] = cos((sensorData.windDir)*(PI/180.0));
  w[1] = sin((sensorData.windDir)*(PI/180.0));
  coord_t currentPosition = {sensorData.lati, sensorData.longi};
  normr = havDist(wayPoints[wpNum], currentPosition);

  //Dummy normal distance
  float oldnormr=1000;

  angleofattack = 15;

  //Optimal angle to go at if we cannot go directly to the waypoint
  //Puts us on a tack or jibe
  //Different values for top and bottom of polar plot
  optpolartop= 45;
  optpolarbot= 40;

  printWaypointData();

  //Reached waypoint!
  if((normr < detectionradius) && ((wpNum + 1) < numWP)){
    printHitWaypointData();
    lightAllLEDs();
    //delay for 3 seconds
    delay(3000);
    lowAllLEDs();
    wpNum += 1 ;

    //reset variables because we have reached the old waypoint
    r[0] = wayPoints[wpNum].longitude - sensorData.longi;
    r[1] = wayPoints[wpNum].latitude - sensorData.lati;
    w[0] = cos((sensorData.windDir)*(PI/180.0));
    w[1] = sin((sensorData.windDir)*(PI/180.0));
    coord_t currentPosition = {sensorData.lati, sensorData.longi};
    normr = havDist(wayPoints[wpNum], currentPosition);

        //Reset all stored boat directions to 0
    for (int i=0; i<numBoatDirReads; i++){
      boatDirections[i] = 0;
    }

    lightAllLEDs();


  }

  lowAllLEDs();

  //dir is the direction to the next waypoint from the boat
  //because we want the angle from the y axis (from the north), we take atan of adjacent over oppoosite
  float anglewaypoint=atan2(r[0],r[1])*360/(2*PI);
  anglewaypoint=convertto360(anglewaypoint);

  float dirangle=anglewaypoint-sensorData.windDir;
  dirangle=convertto360(dirangle);

  float boatDirection = sensorData.boatDir;
//  boatDirection=360-(boatDirection-90);
  boatDirection=convertto360(boatDirection);

  // Wind w.r.t the boat
  float windboat;
  windboat=sensorData.windDir-boatDirection;

  float boat_wrt_wind=boatDirection-sensorData.windDir;
//  boat_wrt_wind=360-(boat_wrt_wind-90);
  boat_wrt_wind=convertto360(boat_wrt_wind);

  float windDir=sensorData.windDir;

  /*---------checking if past maximum tacking width------------*/
  float distance_to_center = center_distance(currentPosition);
  // boat facing right and past line, must turn left
//  if (distance_to_center<max_distance){
//    if (dirangle<180){
//      Serial.println("Past line to the right, turning left");
//      Serial1.println("Past line to the right, turning left");
//      sailAngle=sensorData.windDir-angleofattack;
//      tailAngle=sensorData.windDir;
//    }
//    else {
//      Serial.println("Past line to the left, turning right");
//      Serial1.println("Past line to the left, turning right");
//      sailAngle=sensorData.windDir+angleofattack;
//      tailAngle=sensorData.windDir;
//    }
//  }
/* heading upwind angles
  general format:
    offset is the angle between the boat's actual orientation and desired orientation
    eg when if wind is at 0, opttop is at 45, and we boat is facing 20, offset is 25
    w is wind wrt north
    opttop and optboat are 45 or 40 eg
    boatdir is boat wrt north

  general: if boat is facing right, and cant sail directly to WP or turn to WP,
    tailangle= wind -offset
    facing left: tailangle=wind+offset
  facing right, angle is above in the sector: w-offset
    w-(|w+opttop - boatdir|)
  facing left, angle is below in the sector: w+offset
    w+(|w+180-optboat-boatdir)
  facing left, angle is above in the sector: w+offset
    w+(|w-opttop-boatdir|)
  facing right, angle is below in the sector: w-offset
    w-(|w+180+optbot-boatdir|)
*/
//  boat initially facing right

// the angle that we intend to sail at WRT North
  float intended_angle;
  float intended_angle_of_attack;
  if (boat_wrt_wind<180) {
    //Up right
    if (dirangle<optpolartop && dirangle>0){
      Serial1.println("FACING RIGHT UP RIGHT");
      Serial1.println("\n\nIntended Sector: UP RIGHT\n\n");
      // tailAngle=upRight(boatDirection,windDir);
      // sailAngle=tailAngle+15;
      intended_angle=windDir+optpolartop;
      intended_angle_of_attack=15;
    }
    //Head directly to target to the right
    else if (dirangle>optpolartop && dirangle<180- optpolarbot){
      Serial1.println("FACING RIGHT DIRECT RIGHT");
      Serial1.println("\n\nIntended Sector: DIRECT RIGHT\n\n");
      // tailAngle=rightTarget(boatDirection,windDir);
      // sailAngle=tailAngle+15;
      intended_angle=anglewaypoint;
      intended_angle_of_attack=15;
    }
    //Head directly to target to the left
    else if (dirangle>optpolarbot + 180 && dirangle<360 -optpolartop){
      Serial1.println("FACING RIGHT DIRECT LEFT");
      Serial1.println("\n\nIntended Sector: _________TURN LEFT\n\n");

      // turning
      // THIS IS WHERE WE WILL NEED TO CALL A TURN FUNCTION
      delay(5000);
      // tailAngle=leftTarget(boatDirection,windDir);
      // sailAngle=tailAngle-15;
      intended_angle=anglewaypoint;
      intended_angle_of_attack=-15;
    }
    //Up left
    else if (dirangle>360-optpolartop){
      Serial1.println("FACING RIGHT UP LEFT");
      Serial1.println("\n\nIntended Sector: UP RIGHT\n\n");
      // tailAngle=upRight(boatDirection,windDir);
      // sailAngle=tailAngle+15;
      intended_angle=windDir+optpolartop;
      intended_angle_of_attack=15;
    }
    //bottom left
    else if (dirangle < 180 + optpolarbot && dirangle > 180){
      Serial1.println("FACING RIGHT BOTTOM LEFT");
      Serial1.println("\n\nIntended Sector: BOTTOM RIGHT\n\n");
      // tailAngle=downRight(boatDirection,windDir);
      // sailAngle=tailAngle+15;
      intended_angle=windDir+180-optpolarbot;
      intended_angle_of_attack=15;
    }
    //bottom right
    else {
      Serial1.println("FACING RIGHT BOTTOM RIGHT");
      Serial1.println("\n\nIntended Sector: BOTTOM RIGHT\n\n");
      // tailAngle=downRight(boatDirection,windDir);
      // sailAngle=tailAngle+15;
      intended_angle=windDir+180-optpolarbot;
      intended_angle_of_attack=15;
    }
  }
  //boat facing to left
  else{
    //Up right
    if (dirangle<optpolartop && dirangle>0){
      Serial1.println("FACING LEFT UP RIGHT");
      Serial1.println("\n\nIntended Sector: UP LEFT\n\n");
      // tailAngle=upLeft(boatDirection,windDir);
      // sailAngle=tailAngle-15;
      intended_angle=windDir-optpolartop;
      intended_angle_of_attack=-15;
    }
    //Head directly to target to the right
    else if (dirangle>optpolartop && dirangle<180- optpolarbot){
      Serial1.println("FACING LEFT DIRECT RIGHT");
      Serial1.println("\n\nIntended Sector: _______TURN RIGHT\n\n");
      //THIS IS WHERE WE WILL NEED TO CALL A TURN FUNCTION
      delay(5000);
      // tailAngle=rightTarget(boatDirection,windDir);
      // sailAngle=tailAngle+15;
      intended_angle=anglewaypoint;
      intended_angle_of_attack=15;
    }
    //Head directly to target to the left
    else if (dirangle>optpolarbot + 180 && dirangle<360 -optpolartop){
      Serial1.println("FACING LEFT DIRECT LEFT");
      Serial1.println("\n\nIntended Sector: DIRECT LEFT\n\n");
      // tailAngle=leftTarget(boatDirection,windDir);
      // sailAngle=tailAngle-15;
      intended_angle=anglewaypoint;
      intended_angle_of_attack=-15;
    }
    //Up left
    else if (dirangle>360-optpolartop){
      Serial1.println("FACING LEFT UP LEFT");
      Serial1.println("\n\nIntended Sector: UP LEFT\n\n");
      // tailAngle=upLeft(boatDirection,windDir);
      // sailAngle=tailAngle-15;
      intended_angle=windDir-optpolartop;
      intended_angle_of_attack=-15;
    }
    //bottom left
    else if (dirangle < 180 + optpolarbot && dirangle > 180){
      Serial1.println("FACING LEFT BOTTOM LEFT");
      Serial1.println("\n\nIntended Sector: BOTTOM LEFT\n\n");
      // tailAngle=downLeft(boatDirection,windDir);
      // sailAngle=tailAngle-15;
      intended_angle=windDir+180+optpolarbot;
      intended_angle_of_attack=-15;
    }
    //bottom right
    else {
      Serial1.println("FACING LEFT BOTTOM RIGHT");
      Serial1.println("\n\nIntended Sector: BOTTOM LEFT\n\n");
      // tailAngle=downLeft(boatDirection,windDir);
      // sailAngle=tailAngle-15;
      intended_angle=windDir+180+optpolarbot;
      intended_angle_of_attack=-15;
    }
  }

  // ATTEMPT TO USE INTENDED ANGLE
  float offset=boatDirection-intended_angle;
  Serial1.print("Intended angle: ");
  Serial1.println(intended_angle);
  Serial.print("Intended angle: ");
  Serial.println(intended_angle);
  Serial1.print("Offset from intended angle: ");
  Serial1.println(offset);
  Serial.print("Offset: ");
  Serial.println(offset);

  // use the offset (a positive or negative angle that shows the amount that we
  // are facing away from the intended angle) and wind direction to set the sail
  // then offset the sail to the left or right WRT the tail to give it an angle
  // of attack
  tailAngle=windDir+offset;
  sailAngle=tailAngle+intended_angle_of_attack;

  tailAngle=(float)((int)tailAngle%360);
  tailAngle=tailAngle+360;
  tailAngle=(float)((int)tailAngle%360);

  sailAngle=(float)((int)sailAngle%360);
  sailAngle=sailAngle+360;
  sailAngle=(float)((int)sailAngle%360);

  printSailTailSet();

  //Convert sail and tail from wrt north to wrt boat
  sailAngle=sailAngle-sensorData.boatDir;
  tailAngle=tailAngle-sensorData.boatDir;
  // convert sail to 0-360
  sailAngle = int(sailAngle+360)%360;

  // convert tail to -180-180
  tailAngle = int(tailAngle+360)%360;
  if (tailAngle> 180) {tailAngle -= 360;}

  //Get servo commands from the calculated sail and tail angles
  if (sailAngle < 0) {
    sailAngle += 360;
  }
// REAL BOAT SAIL AND TAIL MAPPING
  // tailAngle = tailMap(sailAngle, tailAngle);
  // sailAngle = sailMap(sailAngle);

// TESTBENCH SAIL AND TAIL MAPPING
  tailAngle = tailMapBench(sailAngle, tailAngle);
  sailAngle = sailMapBench(sailAngle);

  // printServoSailTail();

}

/*Sets servos to the sailAngle and tailAngle*/
void nServos(void) {
  tailServo.write(tailAngle);
  sailServo.write(sailAngle);
}


