/* Minion Constraint Solver
   http://minion.sourceforge.net
   
   For Licence Information see file LICENSE.txt 

   $Id: minion.h 1409 2008-05-21 14:47:25Z caj $
*/

/* Minion
* Copyright (C) 2006
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

//This class encapsulates a data structure that allows you to:
// - locate a datum using its key
// - efficiently access the key (and datum) for the maximum datum
//Both these operations are log(n).
//The datum is provided via add() and it becomes property of the class, i.e., you
//must access it using getData() in future to ensure that the class copy is amended.
//checkAdd() is the same as add, except it does nothing if the key is already present.
//You must not duplicate keys, if you do behaviour is undefined.
//Furthermore if you do call getData() and change the class copy, you must call
//fixOrder() or repair() before using any other method again, or subsequent results are undefined.
//The difference between fixOrder() and repair() is that the former assumes only 1 item
//is out of position, while the latter assume that all are.
//Finally, do not add items anywhere except the shallowest node you intend to use
//the data structure, e.g., at the root node, or only use at one depth.

#define left(x) (2 * x + 1)
#define right(x) (2 * x + 2)
#define parent(x) ((x - 1) / 2)

template<typename Key, typename Data>
class RandomAccessPriorityQ {
 public:

  vector<Key> heap; //heap ordered by decreasing data corresponding to key
  map<Key, pair<Data, size_t> > mapping; //mapping from key to data
  Reversible<size_t> correctSize; //tells us what the heap size should be at current depth
  vector<Key> removed; //vector holding keys removed at greater depths
  size_t lastGetDataPos; //the heap location of the key of the most recent data accessed by getData()

  RandomAccessPriorityQ(StateObj* stateObj) : correctSize(stateObj), lastGetDataPos(0) {}

  Data& getData(Key k) { 
    pair<Data, size_t>& data_pos = mapping.find(k)->second;
    lastGetDataPos = data_pos.second;
    return data_pos.first;
  }

  void pullUp(size_t pos) {
    Key pos_k = heap[pos];
    pair<Data, size_t>& data_pos = mapping.find(pos_k)->second;
    while(true) {
      if(pos == 0) break;
      Key parent_k = heap[parent(pos)];
      pair<Data, size_t>& parent_data_pos = mapping.find(parent_k)->second;
      if(parent_data_pos.first < data_pos.first) {
	heap[pos] = parent_k;
	parent_data_pos.second = pos;
	pos = parent(pos);
      } else
	break;
    }
    heap[pos] = pos_k;
    data_pos.second = pos;
  }

  void pullDown(size_t pos) {
    Key init_k = heap[pos];
    pair<Data, size_t>& init_data_pos = mapping.find(init_k)->second;
    size_t heap_s = heap.size();
    while(true) {
      int left = left(pos);
      if(left >= heap_s) //no children
	break;
      else { //left child at least
	Key left_k = heap[left];
	pair<Data, size_t>& left_data_pos = mapping.find(left_k)->second;
	int right = right(pos);
	if(right >= heap_s) { //no right child
	  if(init_data_pos.first < left_data_pos.first) { //but left child is larger
	    heap[pos] = left_k; //so swap the left child and current around and then stop
	    left_data_pos.second = pos;
	    pos = left;
	  }
	  break;
	} else { //both left and right children
	  Key right_k = heap[right];
	  pair<Data, size_t>& right_data_pos = mapping.find(right_k)->second;
	  if(right_data_pos.first < left_data_pos.first) {
	    if(init_data_pos.first < left_data_pos.first) { //left child is largest
	      heap[pos] = left_k; //so bring it up
	      left_data_pos.second = pos;
	      pos = left(pos);
	    } else {
	      break;
	    }
	  } else {
	    if(init_data_pos.first < right_data_pos.first) { //right child is largest
	      heap[pos] = right_k;
	      right_data_pos.second = pos;
	      pos = right(pos);
	    } else {
	      break;
	    }
	  }
	}
      }
    }
    heap[pos] = init_k; //put into final position
    init_data_pos.second = pos;
  }

  void checkCorrectSize() {
    for(int i = heap.size(); i < correctSize; i++) {
      heap.push_back(removed.back());
      removed.pop_back();
      pullUp(heap.size() - 1);
    }
    D_ASSERT(heap.size() == correctSize);
    if(heap.size() != 0)
      D_ASSERT(checkHeap());
  }

  size_t size() { checkCorrectSize(); return correctSize; }

  void clear() { correctSize = 0; heap.clear(); mapping.clear(); }

  Key getMaxKey() { checkCorrectSize(); return heap[0]; }

  void removeMax() {
    checkCorrectSize();
    removed.push_back(heap[0]);
    heap[0] = heap[heap.size() - 1];
    heap.pop_back();
    --correctSize;
    if(heap.size() != 0) {
      pullDown(0);
      D_ASSERT(checkHeap());
    }
  }

  void fixOrder() { pullUp(lastGetDataPos); pullDown(lastGetDataPos); D_ASSERT(checkHeap()); }

  void repair() {
    const size_t heap_s = heap.size();
    for(int i = 0; i < heap_s; i++)
      pullUp(i);
    D_ASSERT(checkHeap());
  }

  void add(Key k, Data d) {
    checkCorrectSize(); 
    D_ASSERT(mapping.find(k) == mapping.end()); //check that it's a unique key
    ++correctSize;
    mapping.insert(make_pair(k, make_pair(d, 0))); //position is wrong ...
    heap.push_back(k);
    pullUp(heap.size() - 1); //but this function will correct the position
    D_ASSERT(checkHeap());
  }

  void checkAdd(Key k, Data d) {
    if(mapping.find(k) == mapping.end())
      add(k, d);
  }

  bool checkHeap() { checkHeap(0); }

  bool checkHeap(size_t pos) {
    size_t heap_s = heap.size();
    D_ASSERT(pos < heap_s);
    pair<Data,size_t>& data_pos = mapping.find(heap[pos])->second;
    Data& data = data_pos.first;
    D_ASSERT(pos == data_pos.second); //check stored pos is correct
    if(left(pos) >= heap_s) //left not present
      return true; //no children => is a heap
    else //left is present
      if(right(pos) >= heap_s) { //left child only
	bool left_ok = !(data < getData(heap[left(pos)])) && checkHeap(left(pos));
	D_ASSERT(left_ok);
	return left_ok;
      } else { //both children present
	bool both_ok = !(data < getData(heap[left(pos)])) && !(data < getData(heap[right(pos)])) &&
	  checkHeap(left(pos)) && checkHeap(right(pos));
	D_ASSERT(both_ok);
	return both_ok;
      }
  }
};

/* Below is the code that I used to test this ADT. Both require access to
a definition like   

RandomAccessPriorityQ<unsigned, int> test(stateObj);

  cout << "begin rapq BT test" << endl;
  
  bttest.clear();
  
  for(unsigned j = 0; j < 10; j++) {
  bttest.add(j, j);
  }
  
  cout << bttest.heap << endl;
  getMemory(stateObj).backTrack().world_push();
  for(unsigned j = 0; j < 5; j++) {
  bttest.removeMax();
  }
  cout << bttest.heap << endl;
  getMemory(stateObj).backTrack().world_push();
  for(unsigned j = 0; j < 5; j++) {
  bttest.removeMax();
  }
  cout << bttest.size() << endl;
  getMemory(stateObj).backTrack().world_pop();
  cout << bttest.getMax().first << endl;
  getMemory(stateObj).backTrack().world_pop();
  cout << bttest.getMax().first << endl;
  
  cout << "end rapq BT test" << endl;
  
  cout << "begin testing rapq" << endl;
  
  for(unsigned j = 0; j < 30; j++) {
  test.add(j, j);
  }
  
  cout << test.heap << endl;
  
  int nums[] = {6,11,21,2,1,29,30,5,13,12,14,17,20,22,3,4,25,26,10,23,28,18,16,7,27,9,8,15,19,24};
  for(int j = 0; j < 30; j++) {
  test.getData(test.getMax().first) = -nums[j];
  test.fixOrder();
  }
  
  cout << test.heap << endl;
  
  cout << test.getData(test.heap[0]);
  for(unsigned j = 1; j < 30; j++)
  cout << "," <<  test.getData(test.heap[j]);
  cout << endl;
  
  for(int j = 0; j < 30; j++) {
  cout << test.getMax().second << " ";
  test.removeMax();
  }
  cout << endl;
  
  D_ASSERT(test.size() == 0);
  
  cout << "end testing rapq" << endl;
*/
