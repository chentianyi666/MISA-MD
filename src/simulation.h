//
// Created by baihe in 2016-12-22.
//

#ifndef CRYSTAL_MD_SIMULATION_H
#define CRYSTAL_MD_SIMULATION_H

#include <mpi.h>
#include <cstring>
#include <io/io_writer.h>

#include "toml_config.h"
#include "newton_motion.h"
#include "input.h"
#include "potential/eam.h"
#include "atom_dump.h"

class simulation {
public:

    simulation();

    ~simulation();

    /**
     * Denote N as the count of all processors.
     * {@memberof domainDecomposition} will divide the simulation box into N parts,
     * we call each part as a sub-box.
     * And each sub-box will bind to a processor.
     */
    void createDomainDecomposition();

    void createAtoms();

    void prepareForStart();

    void simulate();

    void finalize();

    void output(size_t time_step);

    void abort(int exitcode);

private:
    /**
     * the time steps the program have simulated.
     */
    unsigned long _simulation_time_step;

    /**
     * pointer to config data.
     */
    ConfigValues *pConfigVal;
    AtomDump *dump; // pointer to the atom dump class for output atoms information.
    Domain *_p_domain; //仅rank==0的进程有效
    // GlobalDomain *p_domain;  //仅rank==0的进程有效 // todo ??
    atom *_atom;
    NewtonMotion *_newton_motion;

    input *_input;  // 从文件读取原子坐标,速度信息
    eam *_pot; // eam potential

    bool _finalCheckpoint;
};

#endif //CRYSTAL_MD_SIMULATION_H
