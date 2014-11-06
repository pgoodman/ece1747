#ifndef __MOLECULE_H_
#define __MOLECULE_H_

typedef struct Molecule{
  unsigned int id[3];
  double x, y, z;
  double f_x, f_y, f_z;
} Molecule;

#endif //__MOLECULE_H_
