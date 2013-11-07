# 1 "esdg_src.cl" 1


typedef struct latLong
    {
        float lat;
        float lng;
    } LatLong;

__kernel void NearestNeighbor(__global LatLong *d_locations,
			      __global float *d_distances,
			      const int numRecords,
			      const float lat,
			      const float lng) {
  int globalId = get_global_id(0);
  if (globalId < numRecords) {
    __global LatLong *latLong = d_locations+globalId;
    __global float *dist=d_distances+globalId;
    *dist = (float)sqrt((lat-latLong->lat)*(lat-latLong->lat)+
                        (lng-latLong->lng)*(lng-latLong->lng));
  }
}
