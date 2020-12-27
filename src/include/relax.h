
double smooth_elastic_mesh(MeshT *mesh,int dontoverlap, int keepborder,double orthogonal, double maxx,double maxy);

double smooth_mesh_rubber(MeshT *mesh, MeshT *rubber,double rubberish,
                          int i,int j,int label, int dontoverlap,int keepborder, double orthogonal, double maxx,double maxy);

double smooth_thin_plate(MeshT *mesh,int keepborder, double maxx,double maxy);

double smooth_energy_mesh(int type,MeshT *mesh,int dontoverlap, int keepborder,double orthogonal, double maxx,double maxy);
