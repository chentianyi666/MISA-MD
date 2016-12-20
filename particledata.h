#ifndef PARTICLEDATA_H_
#define PARTICLEDATA_H_

#include <mpi.h>

class particledata {
public:
	// ����һ���������ͣ�����MPI���ݴ���
	static void setMPIType(MPI_Datatype &sendPartType);

	particledata();

	unsigned long id;
	int type;
	double r[3];
	double v[3];
};

#endif /* PARTICLEDATA_H_ */