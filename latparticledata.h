#ifndef LATPARTICLEDATA_H_
#define LATPARTICLEDATA_H_

#include <mpi.h>

class latparticledata {
public:
	// ����һ���������ͣ�����MPI���ݴ���
	static void setMPIType(MPI_Datatype &sendPartType);

	latparticledata();

	int type;
	double r[3];
};

#endif /* LATPARTICLEDATA_H_ */