//#pragma OPENCL EXTENSION cl_khr_byte_addressable_store : enable

typedef struct latLong
    {
        float lat;
        float lng;
    } LatLong;




__kernel void Fan2(__global float *m_dev,
                  __global float *a_dev,
                  __global float *b_dev,
                  const int size,
                  const int t) {
for (__esdg_idy = 0; __esdg_idy < 2; ++__esdg_idy) {
for (__esdg_idx = 0; __esdg_idx < 2; ++__esdg_idx) {

	 int globalId = __builtin_le1_read_group_id_0() * 2 + __esdg_idx;//get_global_id(0);
	 
	 int globalIdx = __builtin_le1_read_group_id_0() * 2 + __esdg_idx;//get_global_id(0);
	 int globalIdy = get_group_id(1) * 2 + __esdg_idy;//get_global_id(1);
      if (globalIdx < size-1-t && globalIdy < size-t) {
         a_dev[size*(globalIdx+1+t)+(globalIdy+t)] -= m_dev[size*(globalIdx+1+t)+t] * a_dev[size*t+(globalIdy+t)];
 	 
 	    if(globalIdy == 0){
 		   b_dev[globalIdx+1+t] -= m_dev[size*(globalIdx+1+t)+(globalIdy+t)] * b_dev[t];
 	    }
 	 }
//   One dimensional
// 	 int globalIdx = globalId % size;
// 	 int globalIdy = globalId / size;
// 	 
// 	 if (globalIdx < size-1-t && globalIdy < size-t) {
//          a_dev[size*(globalIdx+1+t)+(globalIdy+t)] -= m_dev[size*(globalIdx+1+t)+t] * a_dev[size*t+(globalIdy+t)];
// 	 }
// 	 if(globalIdy == 0){
//  		   b_dev[globalIdx+1+t] -= m_dev[size*(globalIdx+1+t)+(globalIdy+t)] * b_dev[t];
//      }
    

__ESDG_END: ;
} __esdg_idx = 0;

} __esdg_idy = 0;
}
