//
// Created by genshen on 2019-03-17.
//

#ifndef CRYSTALMD_INTER_PARTICLE_PACKER_H
#define CRYSTALMD_INTER_PARTICLE_PACKER_H

#include <list>
#include <packer.h>
#include <domain/domain.h>

#include "atom/atom_element.h"
#include "particledata.h"

class InterAtomList;

class InterParticlePacker : public Packer<particledata> {
public:
    explicit InterParticlePacker(const comm::Domain &domain, InterAtomList &inter_atom_list);

    const unsigned long sendLength(const int dimension, const int direction) override;

    /**
     * If some inter atoms get out of box, those atom is no more in current dox of current processor.
     * they should be send to corresponding neighbour processors.
     * The out-of-box atoms will be removed from @memberof inter_atom_list.inter_list in this function.
     *
     * @param buffer the buffer to save the out-of-box atoms.
     * @param send_len the length of data to be send.
     * @param dimension dimension of 3d. @param d values = {0,1,2}
     * @param direction  direction of LOW or HIGH.
     */
    void onSend(particledata buffer[], const unsigned long send_len,
                const int dimension, const int direction) override;

    /**
     *
     * unpack exchanged inter atoms data, and save the inter atoms to local inter atom list.
     *
     * @param buffer the buffer of received data.
     * @param receive_len the data length received, the count of inter atoms in @param buffer
     * @param dimension dimension of 3d. @param d values = {0,1,2}
     * @param direction  direction of LOW or HIGH.
     */
    void onReceive(particledata buffer[], const unsigned long receive_len,
                   const int dimension, const int direction) override;

protegcted:
    const comm::Domain &domain;
    InterAtomList &inter_atom_list;
};


#endif //CRYSTALMD_INTER_PARTICLE_PACKER_H
