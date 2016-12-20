#include "domaindecomposition.h"
#include "particledata.h"
#include "latparticledata.h"
#include "domain.h"
#include <iostream>
using namespace std;

domaindecomposition::domaindecomposition() {
	int period[DIM];
	int reorder;
	int num_procs; // ��������

	// 3ά����
	for (int d = 0; d < DIM; d++)
		period[d] = 1;
	reorder = 1;
	// ��ȡ��������
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	setGridSize(num_procs);
	// �Խ�������
	MPI_Cart_create(MPI_COMM_WORLD, DIM, _gridSize, period, reorder, &_comm);

	// ��ȡ���ؽ��̺š��ѿ�������
	MPI_Comm_rank(_comm, &_rank);
	MPI_Cart_coords(_comm, _rank, DIM, _coords);
	if(_rank == 0){
		cout << "MPI grid dimensions: " << _gridSize[0]<<", "<<_gridSize[1]<<", "<<_gridSize[2] << endl;
		cout << "MPI coordinate of current process: " << _coords[0]<<", "<<_coords[1]<<", "<<_coords[2] << endl;
	}
	// ��ȡ�ھӽ��̺�
	for (int d = 0; d < DIM; d++) {
		MPI_Cart_shift(_comm, d, 1, &_neighbours[d][LOWER], &_neighbours[d][HIGHER]);
	}
	particledata::setMPIType(_mpi_Particle_data);
	latparticledata::setMPIType(_mpi_latParticle_data);
	intersendlist.resize(6);
	interrecvlist.resize(6);
}

domaindecomposition::~domaindecomposition() {
	MPI_Type_free(&_mpi_Particle_data);
	MPI_Type_free(&_mpi_latParticle_data);
}

void domaindecomposition::exchangeAtomfirst(atom* _atom, domain* domain) {

	double ghostlengh[DIM]; // ghost�����С

	for (int d = 0; d < DIM; d++) {
		ghostlengh[d] = _atom->get_ghostlengh(d);
	}
	sendlist.resize(6);
	recvlist.resize(6);

	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	latparticledata* sendbuf[2];
	latparticledata* recvbuf[2];

	// MPIͨ��״̬������
	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
    int iswap = 0;
	for (unsigned short d = 0; d < DIM; d++) {
		// ��ԭ��Ҫ��Խ�����Ա߽�, ԭ���������Ҫ��������
	
		double offsetLower[DIM];
		double offsetHigher[DIM];
		offsetLower[d] = 0.0;
		offsetHigher[d] = 0.0;

		// ���������߽�
		if (_coords[d] == 0)
			offsetLower[d] = domain->getGlobalLength(d);
		// �������Ҳ�߽�
		if (_coords[d] == _gridSize[d] - 1)
			offsetHigher[d] = -domain->getGlobalLength(d);

		for (direction = LOWER; direction <= HIGHER; direction++) {
			// �ҵ�Ҫ���͸��ھӵ�ԭ��
			if(d == 0){
				_atom->getatomx(direction, sendlist);
			}
			else if (d == 1){
				_atom->getatomy(direction, sendlist);
			}
			else{
				_atom->getatomz(direction, sendlist);
			}
            
			double shift = 0.0;
			if (direction == LOWER)
				shift = offsetLower[d];
			if (direction == HIGHER)
				shift = offsetHigher[d];

			// ��ʼ�����ͻ�����
			numPartsToSend[d][direction] = sendlist[iswap].size();
			sendbuf[direction] = new latparticledata[numPartsToSend[d][direction]];
			_atom->pack_send(d, numPartsToSend[d][direction], sendlist[iswap++], sendbuf[direction], shift);
		}

		// �������ھ�ͨ��
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numsend = numPartsToSend[d][direction];
			int numrecv;

			// ����/�Ϸ��Ͳ�����/�½���
			MPI_Isend(sendbuf[direction], numsend, _mpi_latParticle_data, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
		    MPI_Get_count(&status, _mpi_latParticle_data, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new latparticledata[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, _mpi_latParticle_data, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}

		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);
			
			//���յ�������λ����Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_recvfirst(d, direction, recvbuf[direction], recvlist);
 
			// �ͷ�buffer
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

void domaindecomposition::exchangeAtom(atom* _atom, domain* domain) {

	double ghostlengh[DIM]; // ghost�����С

	for (int d = 0; d < DIM; d++) {
		ghostlengh[d] = _atom->get_ghostlengh(d);
	}

	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	latparticledata* sendbuf[2];
	latparticledata* recvbuf[2];

	// MPIͨ��״̬������
	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
    int iswap = 0;
	for (unsigned short d = 0; d < DIM; d++) {
		// ��ԭ��Ҫ��Խ�����Ա߽�, ԭ���������Ҫ��������

		double offsetLower[DIM];
		double offsetHigher[DIM];
		offsetLower[d] = 0.0;
		offsetHigher[d] = 0.0;

		// ���������߽�
		if (_coords[d] == 0)
			offsetLower[d] = domain->getGlobalLength(d);
		// �������Ҳ�߽�
		if (_coords[d] == _gridSize[d] - 1)
			offsetHigher[d] = -domain->getGlobalLength(d);

		for (direction = LOWER; direction <= HIGHER; direction++) {
			double shift = 0.0;
			if (direction == LOWER)
				shift = offsetLower[d];
			if (direction == HIGHER)
				shift = offsetHigher[d];

			// ��ʼ�����ͻ�����
			numPartsToSend[d][direction] = sendlist[iswap].size();
			sendbuf[direction] = new latparticledata[numPartsToSend[d][direction]];
			_atom->pack_send(d, numPartsToSend[d][direction], sendlist[iswap++], sendbuf[direction], shift);
		}

		// �������ھ�ͨ��
		for (direction = LOWER; direction <= HIGHER; direction++) {
		
			int numsend = numPartsToSend[d][direction];
			int numrecv;

			MPI_Isend(sendbuf[direction], numsend, _mpi_latParticle_data, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
		    MPI_Get_count(&status, _mpi_latParticle_data, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new latparticledata[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, _mpi_latParticle_data, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}

		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);
			
			//���յ�������λ����Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_recv(d, direction, numrecv, recvbuf[direction], recvlist);
 
			// �ͷ�buffer
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

void domaindecomposition::exchangeInter(atom* _atom, domain* domain) {
	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	particledata* sendbuf[2];
	particledata* recvbuf[2];

	// MPIͨ��״̬������
	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
	// �ҵ�Ҫ���͸��ھӵ�ԭ��
	for (unsigned short d = 0; d < DIM; d++) {
		for (direction = LOWER; direction <= HIGHER; direction++) {
			numPartsToSend[d][direction] = _atom->getintersendnum(d, direction);
			sendbuf[direction] = new particledata[numPartsToSend[d][direction]];

			_atom->pack_intersend(sendbuf[direction]);
		}

		// �������ھ�ͨ��
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numsend = numPartsToSend[d][direction];
			int numrecv;

			MPI_Isend(sendbuf[direction], numsend, _mpi_Particle_data, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
		    MPI_Get_count(&status, _mpi_Particle_data, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new particledata[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, _mpi_Particle_data, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}

		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);
			
			//���յ�������λ����Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_interrecv(d, numrecv, recvbuf[direction]);
 
			// �ͷ�buffer
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

void domaindecomposition::borderInter(atom* _atom, domain* domain){
	double ghostlengh[DIM]; // ghost�����С

	for (int d = 0; d < DIM; d++) {
		ghostlengh[d] = _atom->get_ghostlengh(d);
	}

	intersendlist.clear();
	interrecvlist.clear();
	intersendlist.resize(6);
	interrecvlist.resize(6);
	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	latparticledata* sendbuf[2];
	latparticledata* recvbuf[2];

	// MPIͨ��״̬������
	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
    int iswap = 0;
	int jswap = 0;
	for (unsigned short d = 0; d < DIM; d++) {
		double offsetLower[DIM];
		double offsetHigher[DIM];
		offsetLower[d] = 0.0;
		offsetHigher[d] = 0.0;

		// ���������߽�
		if (_coords[d] == 0)
			offsetLower[d] = domain->getGlobalLength(d);
		// �������Ҳ�߽�
		if (_coords[d] == _gridSize[d] - 1)
			offsetHigher[d] = -domain->getGlobalLength(d);

		for (direction = LOWER; direction <= HIGHER; direction++) {
			// �ҵ�Ҫ���͸��ھӵ�ԭ��
			_atom->getIntertosend(d, direction, ghostlengh[d], intersendlist[iswap]);
            
			double shift = 0.0;
			if (direction == LOWER)
				shift = offsetLower[d];
			if (direction == HIGHER)
				shift = offsetHigher[d];

			// ��ʼ�����ͻ�����
			numPartsToSend[d][direction] = intersendlist[iswap].size();
			sendbuf[direction] = new latparticledata[numPartsToSend[d][direction]];
			_atom->pack_bordersend(d, numPartsToSend[d][direction], intersendlist[iswap++], sendbuf[direction], shift);

		}

		// �������ھ�ͨ��
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numsend = numPartsToSend[d][direction];
			int numrecv;

			MPI_Isend(sendbuf[direction], numsend, _mpi_latParticle_data, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
		    MPI_Get_count(&status, _mpi_latParticle_data, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new latparticledata[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, _mpi_latParticle_data, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}

		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			interrecvlist[jswap].resize(numrecv);
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);
			
			//���յ�������λ����Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_borderrecv(numrecv, recvbuf[direction], interrecvlist[jswap++]);
 
			// �ͷ�buffer
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

void domaindecomposition::sendrho(atom* _atom){
	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	double* sendbuf[2];
	double* recvbuf[2];

	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
	int iswap = 5;
	for (int d = (DIM - 1); d >= 0; d--){
		for (direction = LOWER; direction <= HIGHER; direction++){
			numPartsToSend[d][direction] = recvlist[iswap].size();
			sendbuf[direction] = new double[numPartsToSend[d][direction]];
			_atom->pack_rho(numPartsToSend[d][direction], recvlist[iswap--], sendbuf[direction]);
		}
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numsend = numPartsToSend[d][direction];
			int numrecv;

			MPI_Isend(sendbuf[direction], numsend, MPI_DOUBLE, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
			MPI_Get_count(&status, MPI_DOUBLE, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new double[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, MPI_DOUBLE, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);

			//���յ��ĵ������ܶ���Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_rho(d, direction, recvbuf[direction], sendlist);

			// �ͷ�buffer
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

void domaindecomposition::sendDfEmbed(atom* _atom){
	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	double* sendbuf[2];
	double* recvbuf[2];

	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
    int iswap = 0;
	int jswap = 0;
	for (unsigned short d = 0; d < DIM; d++) {
		for (direction = LOWER; direction <= HIGHER; direction++) {
			// ��ʼ�����ͻ�����
			numPartsToSend[d][direction] = sendlist[iswap].size() + intersendlist[iswap].size();
			sendbuf[direction] = new double[numPartsToSend[d][direction]];
			_atom->pack_df(sendlist[iswap], intersendlist[iswap], sendbuf[direction]);
			iswap++;
		}

		// �������ھ�ͨ��
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numsend = numPartsToSend[d][direction];
			int numrecv;

			MPI_Isend(sendbuf[direction], numsend, MPI_DOUBLE, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
		    MPI_Get_count(&status, MPI_DOUBLE, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new double[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, MPI_DOUBLE, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}

		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);
			
			//���յ���Ƕ���ܵ�����Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_df(numrecv, recvbuf[direction], recvlist[jswap], interrecvlist[jswap]);
			jswap++;
 
			// �ͷ�buffer
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

void domaindecomposition::sendforce(atom* _atom){
	// ���͡��������ݻ�����
	int numPartsToSend[DIM][2];
	int numPartsToRecv[DIM][2];
	double* sendbuf[2];
	double* recvbuf[2];

	MPI_Status status;
	MPI_Status send_statuses[DIM][2];
	MPI_Status recv_statuses[DIM][2];
	MPI_Request send_requests[DIM][2];
	MPI_Request recv_requests[DIM][2];

	int direction;
    int iswap = 5;
	for (int d = (DIM - 1); d >= 0; d--){
		for (direction = LOWER; direction <= HIGHER; direction++){
			numPartsToSend[d][direction] = recvlist[iswap].size();
			sendbuf[direction] = new double[numPartsToSend[d][direction] * 3];
			_atom->pack_force(numPartsToSend[d][direction], recvlist[iswap--], sendbuf[direction]);
		}
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numsend = numPartsToSend[d][direction] * 3;
			int numrecv;

			MPI_Isend(sendbuf[direction], numsend, MPI_DOUBLE, _neighbours[d][direction], 99, _comm, &send_requests[d][direction]);
			MPI_Probe(_neighbours[d][(direction + 1) % 2], 99, _comm, &status);//�����ھ��Ƿ�����Ϣ���͸�����
		    MPI_Get_count(&status, MPI_DOUBLE, &numrecv);//�õ�Ҫ���յ�������Ŀ
			// ��ʼ�����ջ�����
			//���ݵõ����ͷ�Ҫ����������Ϣ��С����ʼ�����ջ�����
			recvbuf[direction] = new double[numrecv];
			numPartsToRecv[d][direction] = numrecv;
			MPI_Irecv(recvbuf[direction], numrecv, MPI_DOUBLE, _neighbours[d][(direction + 1) % 2], 99, _comm, &recv_requests[d][direction]);
		}
		for (direction = LOWER; direction <= HIGHER; direction++) {
			int numrecv = numPartsToRecv[d][direction];
			MPI_Wait(&send_requests[d][direction], &send_statuses[d][direction]);
			MPI_Wait(&recv_requests[d][direction], &recv_statuses[d][direction]);
			
			//���յ�������λ����Ϣ�ӵ���Ӧ�洢λ����
			_atom->unpack_force(d, direction, recvbuf[direction], sendlist);
 
			delete[] sendbuf[direction];
			delete[] recvbuf[direction];
		}
	}
}

double domaindecomposition::getBoundingBoxMin(int dimension, domain* domain) {
	return _coords[dimension] * domain->getGlobalLength(dimension) / _gridSize[dimension];
}

double domaindecomposition::getBoundingBoxMax(int dimension, domain* domain) {
	return (_coords[dimension] + 1) * domain->getGlobalLength(dimension) / _gridSize[dimension];
}

void domaindecomposition::setGridSize(int num_procs) {
	for( int i = 0; i < DIM; i++ )
	_gridSize[i] = 0;
	MPI_Dims_create( num_procs, DIM, (int *) &_gridSize );
}