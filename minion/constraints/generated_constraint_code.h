//  Call buildtree
//  Count:  100
//  Matched:  10
//  Matched:  20
//  Count:  200
//  Matched:  40
//  Tree cost: 166
//  Better tree found, of size:166
#ifdef PREPARE
#define SYMMETRIC
#else
int get_mapping_size() { return 16; }
vector<int> get_mapping_vector() {
  vector<int> v;
  v.push_back(0);   v.push_back(-1);
  v.push_back(0);   v.push_back(1);
  v.push_back(1);   v.push_back(-1);
  v.push_back(1);   v.push_back(1);
  v.push_back(2);   v.push_back(-1);
  v.push_back(2);   v.push_back(1);
  v.push_back(3);   v.push_back(-1);
  v.push_back(3);   v.push_back(1);
  v.push_back(4);   v.push_back(-1);
  v.push_back(4);   v.push_back(1);
  v.push_back(5);   v.push_back(-1);
  v.push_back(5);   v.push_back(1);
  v.push_back(6);   v.push_back(-3);
  v.push_back(6);   v.push_back(-1);
  v.push_back(6);   v.push_back(1);
  v.push_back(6);   v.push_back(3);
  return v; 
}
virtual void full_propagate() 
{

}
#endif

//  Depth: 17
//  Number of nodes: 166
//  Number of nodes explored by algorithm: 315
//Group Size: 768
