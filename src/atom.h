//
// Created by baihe back to 2016-01-06.
//

#ifndef CRYSTAL_MD_ATOM_H
#define CRYSTAL_MD_ATOM_H

#include <cstdio>
#include <vector>
#include <io/io_writer.h>

#include "pre_define.h"
#include "domain.h"
#include "eam.h"
#include "particledata.h"
#include "lat_particle_data.h"

class Domain; // todo remove.

class atom {
public :
    friend class WorldBuilder;
    friend class AtomDump;

    atom(Domain *domain, double latticeconst,
         double cutoffRadiusFactor, int seed);

    ~atom();

    void addAtom(unsigned long id, double rx, double ry, double rz, double vx, double vy, double vz);

    int decide();

    void clearForce();

    void computeEam(eam *pot, Domain *domain, double &comm);

    int getinteridsendsize();

    void computefirst(double dtInv2m, double dt);

    void computesecond(double dtInv2m);

    void getatomx(int direction, vector<vector<int>> &sendlist);

    void getatomy(int direction, vector<vector<int>> &sendlist);

    void getatomz(int direction, vector<vector<int>> &sendlist);

    void getIntertosend(int d, int direction, double ghostlengh, vector<int> &sendlist);

    int getintersendnum(int dimension, int direction);

    void pack_intersend(particledata *buf);

    void unpack_interrecv(int d, int n, particledata *buf);

    void pack_bordersend(int dimension, int n, vector<int> &sendlist, LatParticleData *buf, double shift);

    void unpack_borderrecv(int n, LatParticleData *buf, vector<int> &recvlist);

    void pack_send(int dimension, int n, vector<int> &sendlist, LatParticleData *buf, double shift);

    void unpack_recvfirst(int d, int direction, int n, LatParticleData *buf, vector<vector<int> > &recvlist);

    void unpack_recv(int d, int direction, int n, LatParticleData *buf, vector<vector<int>> &recvlist);

    void pack_rho(int n, vector<int> &recvlist, double *buf);

    void unpack_rho(int d, int direction, double *buf, vector<vector<int>> &sendlist);

    void pack_df(vector<int> &sendlist, vector<int> &intersendlist, double *buf);

    void unpack_df(int n, double *buf, vector<int> &recvlist, vector<int> &interrecvlist);

    void pack_force(int n, vector<int> &recvlist, double *buf);

    void unpack_force(int d, int direction, double *buf, vector<vector<int>> &sendlist);

    void setv(int lat[4], double collision_v[3]);

    int getnlocalatom();

    void print_force();

private:
    void calculateNeighbourIndices();

    long IndexOf3DIndex(long int xIndex, long int yIndex, long int zIndex) const;

//    double uniform();

    Domain *p_domain;
    int numberoflattice;

    double _cutoffRadius;
    int _cutlattice;
    double _latticeconst;
    int _seed;

    vector<long int> NeighbourOffsets; // 邻居粒子偏移量

    //晶格点原子用数组存储其信息
    _type_atom_id *id; // including ghost atoms.
    int *type;
    double *x, *v, *f, *rho, *df;

    vector<unsigned long> idinter;
    vector<int> typeinter;
    vector<vector<double>> xinter; // 间隙原子坐标
    vector<vector<double>> vinter; // 间隙原子速度
    vector<vector<double>> finter; // 间隙原子力
    vector<double> rhointer;
    vector<double> dfinter;
    int nlocalinter, nghostinter; // 本地间隙原子数和ghost间隙原子数

    vector<unsigned long> interbuf;
};

#endif // CRYSTAL_MD_ATOM_H
