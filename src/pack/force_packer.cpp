//
// Created by genshen on 2019-03-16.
//

#include "force_packer.h"

ForcePacker::ForcePacker(AtomList &atom_list, std::vector<std::vector<_type_atom_id>> &send_list,
                         std::vector<std::vector<_type_atom_id>> &receive_list)
        : atom_list(atom_list), send_list(send_list), receive_list(receive_list) {}

const unsigned long ForcePacker::sendLength(const int dimension, const int direction) {
    const int index = 2 * dimension + (direction == LOWER ? 1 : 0);
    return receive_list[index].size() * 3;
}

void ForcePacker::onSend(double *buffer, const unsigned long send_len, const int dimension, const int direction) {
    const int index = 2 * dimension + (direction == LOWER ? 1 : 0);
    std::vector<_type_atom_id> &recvlist = receive_list[index];
    int j, m = 0;
    for (int i = 0; i < send_len / 3; i++) {
        j = recvlist[i];
        AtomElement &atom = atom_list.getAtomEleByLinearIndex(j);
        buffer[m++] = atom.f[0];
        buffer[m++] = atom.f[1];
        buffer[m++] = atom.f[2];
    }
}

void ForcePacker::onReceive(double *buffer, const unsigned long receive_len, const int dimension, const int direction) {
//    const int index = 2 * dimension + (direction == LOWER ? 1 : 0);
    //将收到的粒子位置信息加到对应存储位置上
    double *buf = buffer;
    std::vector<std::vector<_type_atom_id> > &sendlist = send_list;
    const int list_index = 2 * dimension + (direction == LOWER ? HIGHER : LOWER); // Flip the direction
    int j, m = 0;
    for (int i = 0; i < sendlist[list_index].size(); i++) {
        j = sendlist[list_index][i];
        AtomElement &atom_ = atom_list.getAtomEleByLinearIndex(j);
        atom_.f[0] += buf[m++];
        atom_.f[1] += buf[m++];
        atom_.f[2] += buf[m++];
    }
}
