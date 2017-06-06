#include <Arduino.h>
#include <math.h>
#include <Servo.h>
#include "print.h"
#include "sensors.h"
#include "navigation.h"
#include "navigation_helper.h"

//Define Servo types for Sail and Tail
Servo tailServo;
Servo sailServo;

/*----------Navigation Variables----------*/
unsigned long  milTime; //Miliseconds since program started
int wpNum; //the current waypoint's number in the wayPoints array
int numWP; //total number of waypoints on current course
float detectionradius = detectionRadius; //how far away the boat marks a waypoint "reached"
float optpolartop = optPolarTop;
float optpolarbot = optPolarBot;
float angleofattack = angleOfAttack;
coord_t wayPoints[maxPossibleWaypoints]; //the array containing the waypoints
float normr; //normal distance to the waypoint
float r[2]; //r[0]: Longitude difference b/w current position and waypoint,
            //r[1]: Lattitude difference b/w current position and waypoint
float w[2]; //w[0]: Cosine of the wind direction w.r.t North,
            //w[1]: Sine of the wind direction w.r.t North
float sailAngle;
float tailAngle;
// the angle that we intend to sail at WRT North

float intended_angle;
float intended_angle_of_attack;
// latitude is y, longitude is x
float max_distance=100;
coord_t center_start={1,2};
coord_t center_end={10,20};

//set to 0 if we are not doing station keeping or 1 if we are
bool stationKeeping = 0;;

float slope=(center_end.latitude - center_start.latitude)/(center_end.longitude - center_start.longitude);
float intercept= center_start.latitude - slope * center_start.longitude;

// need to read these values from the arduino
float time_inside=0;
// time required to stay inside the box for station keeping
// 5 minutes*60 seconds*1000millis=300,000
float time_req=420000;

// 3 minutes*60 seconds*1000millis=300,000
float max_turn_time=180000;
float start_turn_time=0;
float start_box_time;
// 10 seconds*1000 millis=10000
float time_to_turn=18000;
float prev_intended_angle=270;
float prev_intended_angle_of_attack=10;
bool turning=false;
bool turn_finished=true;
bool start_end_wp=false;
bool continue_end_wp=false;
float final_intended_angle;


// float left_bank=42

/* takes a coordinate and returns the distance from the coordinate to the center line */
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
  milTime = millis();
}

/*Sets servos to the sailAngle and tailAngle*/
void nServos(void) {
  tailServo.write(tailAngle);
  sailServo.write(sailAngle);
}

void updateTime(void){
  milTime = millis();
}

void startTurn(float init_intended, float init_angle, unsigned long start_time){
  turning=true;
  prev_intended_angle=init_intended;
  prev_intended_angle_of_attack=init_angle;
  start_turn_time=start_time;
}

void holdTurn(){
  intended_angle=prev_intended_angle;
  intended_angle_of_attack=prev_intended_angle_of_attack;
}

void finishTurn(){
  turning=false;
  turn_finished=true;
}

/*----------Stored Coordinates----------*/
//Coordinates in and around the Engineering Quad, Cornell university
coord_t olin = {42.445457, -76.484322}; //Olin Hall
coord_t hollister = {42.444259, -76.484435}; //Hollister Hall
coord_t outsideDuffield = {42.444254, -76.482660}; //Outside West entrance to Duffield Hall, atop the stairs
coord_t outsideThurston = {42.444228, -76.483666}; //In front of Thurston Hall
coord_t engQuadX = {42.444612, -76.483492}; //Center of Engineering Quad
coord_t northBottomStairs = {42.444735, -76.483275}; //Bottom of stairs, to the left of North entrance to Duffield Hall
coord_t engQuadRight = {42.4444792, -76.483244}; //Middle of the right sector, looking North, of the engineering quad
coord_t sundial = {42.4448630, -76.4835293}; //Sundial on the engineering quad
coord_t bottomRightIntersection= {42.444242, -76.483247}; //path intersection bottom right of quad
coord_t thurstonWestEntrance={42.444245, -76.484068}; //west entrance to thurston on the path

//Coordinates in and around the Cornell Sailing Center
coord_t lakeOut = {42.469386,-76.504690}; //Out in the lake, to the left of the Cornell Sailing Center
coord_t lakeOut4 = {42.469847,-76.504522}; //Out in the lake, to the left of the Cornell Sailing Center
coord_t lakeOut_beach_far = {42.469065,-76.506674}; //Out in the lake, to the right of the Cornell Sailing Center
coord_t lakeOut_beach = {42.470894,-76.504712}; //Out in the lake, to the right of the Cornell Sailing Center but further North
coord_t lakeOut_beach2 = {42.470535, -76.50489};
coord_t lakeOut_beach3 = {42.471124, -76.505757};
coord_t shore_beach = {42.470862,-76.503950}; //Beach, to the right of the Cornell Sailing Center
coord_t acrossDock = {42.465702, -76.524625}; //Across the lake, when looking from the far edge of the dock to the right of the Cornell Sailing Center
coord_t acrossBeach = {42.467918, -76.525419}; //Across the lake, when looking from the beach to the left of the Cornell Sailing Center
coord_t low_dock={42.468951,-76.502941}; //Right at the lower dock
coord_t high_dock={42.469552,-76.503353}; //High dock launch point

coord_t dirt_mound={42.454386, -76.475693};
coord_t appel_door={42.453915, -76.475891};
coord_t appel_farRight={42.454345, -76.474471};
coord_t appel_topRight={42.454715, -76.474551};
coord_t appel_topLeft={42.454865, -76.475672};
coord_t appel_bottomRight={42.453828, -76.474454};

/*Sets waypoints for navigation
* by creating the wayPoints array*/
void setWaypoints(void) {

  //Make the waypoint array
  numWP = 4;
  wpNum = 0;

  //Set way points to desired coordinates.
  //Assignmment must be of the type coord_t.
  wayPoints[0] = lakeOut;
  wayPoints[1] = lakeOut_beach_far;
  wayPoints[2] = high_dock;
  wayPoints[3] = lakeOut4;
  wayPoints[4] = shore_beach;
  

  // nav test with wind from 340
  /*
  {42.47094, -76.504745}
  {42.470432, -76.505028}
  {42.470318, -76.504906}
  {42.470348, -76.504654}
  {42.470493, -76.504143}
  {42.47065, -76.504028}
  {42.470764, -76.504135}
  {42.47094, -76.504745}
  https://binged.it/2qgjHbs
  */
}

/*----------------------------------------*/
/*----------NAVIGATION ALGORITHM----------*/
/*----------------------------------------*/

/*Uses sensorData.windDir, sensorData.boatDir to set sailAngle and tailAngle.
 * sailAngle and tailAngle are set in terms of servo command numbers, but are first
 * calculated in terms of angle w.r.t the boat direction*/
void nShort(void) {

  /*----------Unit testing setters-----------*/
    //sensorData.lati = 42.4441782290;
    //sensorData.lati=outsideThurston.latitude;
    //sensorData.longi=outsideThurston.longitude;
    //sensorData.longi = -76.4834140241;
    //sensorData.windDir = 270;
    //sensorData.boatDir = 0;
    //sensorData.sailAngleNorth = 90;

  //find the normal distance to the waypoint
  r[0] = wayPoints[wpNum].longitude - sensorData.longi;
  r[1] = wayPoints[wpNum].latitude - sensorData.lati;
  w[0] = cos((sensorData.windDir)*(PI/180.0));
  w[1] = sin((sensorData.windDir)*(PI/180.0));
  coord_t currentPosition = {sensorData.lati, sensorData.longi};
  normr = havDist(wayPoints[wpNum], currentPosition);

  //Dummy normal distance
  float oldnormr=1000;

  printData();
  printWaypointData();

  float anglewaypoint=angleToTarget(sensorData.lati, sensorData.longi, wayPoints[wpNum].latitude, wayPoints[wpNum].longitude);
  anglewaypoint=convertto360(anglewaypoint);


  /*
  when not doing station keeping, set boolean stationKeeping to false

  station keeping code: if we have not reached the required time, keep going
  to the same waypoint over and over (we will never register hitting it)
  set the waypoint to be the middle of the square of the station
  once we reach the allotted time, increment our waypoint counter and
  head to the next waypoint which will be a waypoint which is outside the box
  */
  Serial1.print("Time: ");
  Serial1.println(milTime);
  if (stationKeeping){
    /* plan: we set our first waypoint to be the center of the box w detection radius 20m (40/2)
       if we are going to the first waypoint, set radius to 20. 
       also, store the start time that we hit the waypoint, so we can accurately stay inside
       the box for 5 minutes
       once we hit, increase the waypoint number to 1, which will make our boat continue to
       sail to the center of the box
       once 5 minutes are up, increase the waypoint number to 2 and set station keeping to false
       in this way we will sail out the box to wp2 which is outisde the box
    */
    if (wpNum==0){
      detectionradius=20;
      if(normr < detectionradius){
        printHitWaypointData();
        // where we will determine where we aim to go at the end of the required amount of time
        wpNum += 1 ;

        //reset variables because we have reached the old waypoint
        r[0] = wayPoints[wpNum].longitude - sensorData.longi;
        r[1] = wayPoints[wpNum].latitude - sensorData.lati;
        w[0] = cos((sensorData.windDir)*(PI/180.0));
        w[1] = sin((sensorData.windDir)*(PI/180.0));
        currentPosition = {sensorData.lati, sensorData.longi};
        normr = havDist(wayPoints[wpNum], currentPosition);
        start_box_time=milTime;
      }
    }
    //we have  reached the 5 minute time (current time - the time we entered the box at)
    else if(milTime- start_box_time >= time_req ){
      printHitWaypointData();
      // where we will determine where we aim to go at the end of the required amount of time
      wpNum += 1 ;

      //reset variables because we have reached the old waypoint
      r[0] = wayPoints[wpNum].longitude - sensorData.longi;
      r[1] = wayPoints[wpNum].latitude - sensorData.lati;
      w[0] = cos((sensorData.windDir)*(PI/180.0));
      w[1] = sin((sensorData.windDir)*(PI/180.0));
      currentPosition = {sensorData.lati, sensorData.longi};
      normr = havDist(wayPoints[wpNum], currentPosition);
      stationKeeping=0;
    }

  }
  else{
  //----------REACHED WAYPOINT!----------
    // if(((wpNum + 1) < numWP)&&(normr < detectionradius)){
    //   printHitWaypointData();
    //   lightAllLEDs();
    //   //delay for 3 seconds
    //   delay(3000);
    //   lowAllLEDs();
    //   wpNum += 1 ;

    //   //reset variables because we have reached the old waypoint
    //   r[0] = wayPoints[wpNum].longitude - sensorData.longi;
    //   r[1] = wayPoints[wpNum].latitude - sensorData.lati;
    //   w[0] = cos((sensorData.windDir)*(PI/180.0));
    //   w[1] = sin((sensorData.windDir)*(PI/180.0));
    //   currentPosition = {sensorData.lati, sensorData.longi};
    //   normr = havDist(wayPoints[wpNum], currentPosition);
    // }
   //------------------------------------
    if(normr < detectionradius){
      if ((wpNum + 1) < numWP){
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
        currentPosition = {sensorData.lati, sensorData.longi};
        normr = havDist(wayPoints[wpNum], currentPosition);
      }
      else{
        // if we are at the last waypoint, start to save the sail and tail angles so we continue in a straight line
        if (!start_end_wp){start_end_wp=true;}
      }
    }
  }

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
  if (boat_wrt_wind<180) {
    //Up right
    if (dirangle<optpolartop && dirangle>0){
      Serial1.println("FACING RIGHT UP RIGHT");
      Serial1.println("\n\nIntended Sector: UP RIGHT\n\n");
      intended_angle= windDir + optpolartop;
      intended_angle_of_attack = 15;
    }
    //Head directly to target to the right
    else if (dirangle>optpolartop && dirangle<180- optpolarbot){
      Serial1.println("FACING RIGHT DIRECT RIGHT");
      Serial1.println("\n\nIntended Sector: DIRECT RIGHT\n\n");
      intended_angle = anglewaypoint;
      intended_angle_of_attack = 15;
    }
    //Head directly to target to the left
    else if (dirangle>optpolarbot + 180 && dirangle<360 -optpolartop){
      Serial1.println("FACING RIGHT DIRECT LEFT");
      Serial1.println("\n\nIntended Sector: _________TURN LEFT\n\n");
      // turning
      // THIS IS WHERE WE WILL NEED TO CALL A TURN FUNCTION
//      delay(5000);
      intended_angle = anglewaypoint;
      intended_angle_of_attack = -15;
    }
    //Up left
    else if (dirangle>360-optpolartop){
      Serial1.println("FACING RIGHT UP LEFT");
      Serial1.println("\n\nIntended Sector: UP RIGHT\n\n");
      intended_angle = windDir + optpolartop;
      intended_angle_of_attack = 15;
    }
    //bottom left
    else if (dirangle < 180 + optpolarbot && dirangle > 180){
      Serial1.println("FACING RIGHT BOTTOM LEFT");
      Serial1.println("\n\nIntended Sector: BOTTOM RIGHT\n\n");
      intended_angle = windDir + 180 - optpolarbot;
      intended_angle_of_attack = 15;
    }
    //bottom right
    else {
      Serial1.println("FACING RIGHT BOTTOM RIGHT");
      Serial1.println("\n\nIntended Sector: BOTTOM RIGHT\n\n");
      intended_angle = windDir + 180 - optpolarbot;
      intended_angle_of_attack = 15;
    }
  }
  //boat facing to left
  else{
    //Up right
    if (dirangle<optpolartop && dirangle>0){
      Serial1.println("FACING LEFT UP RIGHT");
      Serial1.println("\n\nIntended Sector: UP LEFT\n\n");
      intended_angle = windDir - optpolartop;
      intended_angle_of_attack = -15;
    }
    //Head directly to target to the right
    else if (dirangle>optpolartop && dirangle<180- optpolarbot){
      Serial1.println("FACING LEFT DIRECT RIGHT");
      Serial1.println("\n\nIntended Sector: _______TURN RIGHT\n\n");
      //THIS IS WHERE WE WILL NEED TO CALL A TURN FUNCTION
//      delay(5000);
      intended_angle = anglewaypoint;
      intended_angle_of_attack = 15;
    }
    //Head directly to target to the left
    else if (dirangle>optpolarbot + 180 && dirangle<360 -optpolartop){
      Serial1.println("FACING LEFT DIRECT LEFT");
      Serial1.println("\n\nIntended Sector: DIRECT LEFT\n\n");
      intended_angle = anglewaypoint;
      intended_angle_of_attack = -15;
    }
    //Up left
    else if (dirangle>360-optpolartop){
      Serial1.println("FACING LEFT UP LEFT");
      Serial1.println("\n\nIntended Sector: UP LEFT\n\n");
      intended_angle = windDir - optpolartop;
      intended_angle_of_attack = -15;
    }
    //bottom left
    else if (dirangle < 180 + optpolarbot && dirangle > 180){
      Serial1.println("FACING LEFT BOTTOM LEFT");
      Serial1.println("\n\nIntended Sector: BOTTOM LEFT\n\n");
      intended_angle = windDir + 180 + optpolarbot;
      intended_angle_of_attack = -15;
    }
    //bottom right
    else {
      Serial1.println("FACING LEFT BOTTOM RIGHT");
      Serial1.println("\n\nIntended Sector: BOTTOM LEFT\n\n");
      intended_angle = windDir + 180 + optpolarbot;
      intended_angle_of_attack = -15;
    }
  }

  // turning code


/*
//figure out if we're at the start and need to reset finished or if we have actually completed a turn
if (milTime%turntimereq>turntime && turnfinished){
  //if we havent turned yet (aka turntimereq hasnt happened yet)
  if (milTime%turntimereq - time_to_turn<0){
    turnfinished=true;
  }
}
if (milTime%turntimereq>turntime && !turnfinished && !turning){
  turning=true;
  if facing left{
    sail and tail = upright angle
  }
  else { 
    sail and tail= upleft angle
  }
}

  if ((turning && !turnfinished) && (start_turn_time+time_to_turn)<milTime){
    turning=false;
    turnfinished=true;
}


*/
  // need to know if we are at the start of a turn or the end
  if (turn_finished){
    float t=(int)millis%((int)max_turn_time)-time_to_turn;
    if (t<0){
      turn_finished=false;
    }
  }
  // if we havent finished a turn yet and we are not turning, need to turn
  else if (!turn_finished && !turning){
    // we turn by storing the intended angle in the opposite direction as we are
    // currently facing
    if (boat_wrt_wind<180) {
      //Up right so need to go up left
      if (dirangle<optpolartop && dirangle>0){
        intended_angle= windDir - optpolartop;
        intended_angle_of_attack = -15;
      }
      //Up left so need to go up left
      else if (dirangle>360-optpolartop){
        intended_angle = windDir - optpolartop;
        intended_angle_of_attack = -15;
      }
      //bottom left so need to go bottom left
      else if (dirangle < 180 + optpolarbot && dirangle > 180){
        intended_angle = windDir + 180 + optpolarbot;
        intended_angle_of_attack = -15;
      }
      //bottom right so need to go bottom left
      else if (dirangle > 180 - optpolarbot &&  dirangle < 180){
        intended_angle = windDir + 180 + optpolarbot;
        intended_angle_of_attack = -15;
      }
      // we can head directly to the waypoint left or right so complete the turn
      else{
        finishTurn();
      }
    }
    //boat facing to left
    else{
      //Up right so need to go up right
      if (dirangle<optpolartop && dirangle>0){
        intended_angle = windDir + optpolartop;
        intended_angle_of_attack = 15;
      }
      //Up left so need to go up right
      else if (dirangle>360-optpolartop){
        intended_angle = windDir + optpolartop;
        intended_angle_of_attack = 15;
      }
      //bottom left so need to go bottom right
      else if (dirangle < 180 + optpolarbot && dirangle > 180){
        intended_angle = windDir + 180 - optpolarbot;
        intended_angle_of_attack = 15;
      }
      //bottom right so need to go bottom right
      else if (dirangle > 180 - optpolarbot && dirangle < 180){
        intended_angle = windDir + 180 - optpolarbot;
        intended_angle_of_attack = 15;
      }
      // we can head directly to the waypoint left or right so complete the turn
      else{
        finishTurn();
      }

    }
    // store this intended angle
    startTurn(intended_angle,intended_angle_of_attack, (int)millis);
  }
 // finished a turn
  else if (turning && !turn_finished && (int)start_turn_time+(int)time_to_turn<(int)millis){
    finishTurn();
  }
  // we dont need to reset any variables, so continue with the turn
  else if (turning){
    holdTurn();
  }


// if we have hit the last waypoint, continue at the same intended angle
  if (start_end_wp){
    final_intended_angle=intended_angle;
    continue_end_wp=true;
  }
  if (continue_end_wp){
    intended_angle=final_intended_angle;
  }

  // USE INTENDED ANGLE
  float offset = boatDirection-intended_angle;
  Serial1.print("Intended angle: ");
  Serial1.println(intended_angle);
  Serial1.print("Offset from intended angle: ");
  Serial1.println(offset);

  // Use the offset (a positive or negative angle that shows the amount that we
  // are facing away from the intended angle) and wind direction to set the sail
  // then offset the sail to the left or right w.r.t the tail to give it an angle
  // of attack
  tailAngle = windDir + offset;
  sailAngle = tailAngle + intended_angle_of_attack;

  tailAngle = (float)((int)tailAngle%360);
  tailAngle = tailAngle + 360;
  tailAngle = (float)((int)tailAngle%360);

  sailAngle = (float)((int)sailAngle%360);
  sailAngle = sailAngle+360;
  sailAngle = (float)((int)sailAngle%360);

  //Convert sail and tail from wrt north to wrt boat
  sailAngle = sailAngle - sensorData.boatDir;
  tailAngle = tailAngle - sensorData.boatDir;

  // convert sail to 0-360
  sailAngle = int(sailAngle+360)%360;

//  Serial1.println("Angles w.r.t Boat:");

  //obstacle avoidance; still needs tuning
//  avoidObject();
//
//  Serial1.println("Angles w.r.t Boat after pixy correction:");
//  printSailTailSet();

  // convert tail to -180-180
  tailAngle = int(tailAngle+360)%360;
  if (tailAngle> 180) {tailAngle -= 360;}

  //Get servo commands from the calculated sail and tail angles
  if (sailAngle < 0) {
    sailAngle += 360;
  }

  printSailTailSet();

  sensorData.sailAngleBoat = sailAngle;
  sensorData.tailAngleBoat = tailAngle;

// REAL BOAT SAIL AND TAIL MAPPING
   tailAngle = tailMap(sailAngle, tailAngle);
   sailAngle = sailMap(sailAngle);

// TESTBENCH SAIL AND TAIL MAPPING
//  tailAngle = tailMapBench(sailAngle, tailAngle);
//  sailAngle = sailMapBench(sailAngle);

}





