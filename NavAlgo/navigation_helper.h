/* ************************************************************************** */
/** Navigation Helper Functions
/* ************************************************************************** */

#include "sensors.h"
#include "coordinates.h"

#ifndef NAV_HELPER_H    /* Guard against multiple inclusion */
#define NAV_HELPER_H

typedef struct navigationDataStructure {
    float distToWaypoint; // distance to the next waypoint
    float angleToWaypoint; // angle to the next waypoint
    int currentWaypoint; // index of current waypoint
} nav_t;

extern nav_t *navData;
extern coord_xy *waypoints;

/*Creates origin for XY plane and scales to meters*/
void setOrigin(coord_t *startPoint);

/*Converts coordinate in latitude and longitude to xy*/
void xyPoint(coord_xy *pt, coord_t *latlong);

void navigationInit();

/*finds the distance between two xy points*/
double xyDist(coord_xy *point1, coord_xy *point2);

/*Returns angle (with respect to North) between two global coordinates.*/
float angleToTarget(coord_xy *coord1, coord_xy *coord2);

/*Calculates slope between point1 and point2, designed for use with tacking boundaries */
float xySlope(coord_xy *point1, coord_xy *point2);

void middlePoint(coord_xy *midpt, coord_xy *point1, coord_xy *point2);

// converts an angle to a 0-360 range
float convertto360(float angle);

// finds closest way point to current location given array of way points
void find_closest_waypoint(coord_xy *pt, coord_xy *c, coord_xy waypoints[]);

char* find_point_of_sail (double angle);

char* find_tack (double angle);

double calculate_vmg (double angleToTarget, double boatSpeed);

double true_wind (double windAngle, double windSpeed, double boatSpeed);

void diff(coord_xy *d, coord_xy *T, coord_xy *B);

double fPolar (double windSpeed, double angle);

double calculateAngle(void);

void setServoAngles(double angleToSail);

#endif