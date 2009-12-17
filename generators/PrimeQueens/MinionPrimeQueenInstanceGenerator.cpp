// MinionPrimeQueenInstanceGenerator.cpp

#include "../InstanceHelp.h"
#include "MinionPrimeQueenInstanceGenerator.h"

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Generate
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generate(int n) {
  generateVars(n) ;
  generateVarOrder(n) ;
  generateValOrder(n) ;
  generateMatrices(n) ;
  generateTuples(n) ;
  cout << endl << "objective maximising x" 
       << indexOfFirstBoundsVar <<  endl ;
  cout << endl << "print m1" << endl;
  generateConstraints(n) ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVars
//one var represents queen position
//n^2 vars representing position of each value 1..n^2
//one 0/1 var for each prime.
//one var to contain sum of above 0/1 vars.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateVars(int n) {
  noPrimesInThisInstance = 0 ;
  for (int primesIndex = 0; primesIndex < primesLength; primesIndex++) {
    if (primes[primesIndex] < n*n) noPrimesInThisInstance++ ;
    else break ;
  }
  // One 0/1 var per prime
  cout << noPrimesInThisInstance << endl ;
  indexOfFirstBoundsVar = noPrimesInThisInstance ;
  // 1 bounds var to contain sum of attacked primes
  cout << 1 << endl ;
  cout << 0 << " " << noPrimesInThisInstance << " " << 1 << endl;
  indexOfFirstSparseBoundsVar = indexOfFirstBoundsVar + 1 ;
  // No sparse bounds vars
  cout << 0 << endl ;
  indexOfFirstDiscreteVar = indexOfFirstSparseBoundsVar ;
  // n^2 discrete vars for position of each number, 1 for queen
  cout << n*n+1 << endl ;
  cout << 0 << " " << (n*n-1) << " " << (n*n+1) << endl ;
  indexOfFirstSparseDiscreteVar = indexOfFirstDiscreteVar + (n*n+1) ;
  // No sparse discrete vars
  cout << 0 << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateVarOrder
// Queen is highly constrained, so assign her first.
// The assign each number a position in turn (should do primes 1st).
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateVarOrder(int n) {
  // Put Queen last
  cout << endl << "[" ;
  for (int valueIndex = 0; valueIndex < n*n; valueIndex++)
    cout << "x" << (valueIndex+indexOfFirstDiscreteVar) << ", " ;
    cout << "x" << indexOfFirstDiscreteVar+(n*n) << "]" ;
  // Put Queen 1st
  /*cout << endl << "[" << "x" << indexOfFirstDiscreteVar + (n*n) << " " ;
  for (int valueIndex = 0; valueIndex < n*n; valueIndex++)
    cout << ", x" << (valueIndex+indexOfFirstDiscreteVar) << " " ;
    cout << "]" << endl ;*/
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateValOrder
// Ascending
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateValOrder(int n) {
  cout << endl << "[ a" ;
  for (int i = 0; i < n*n; i++)
    cout << ", a " ;
  cout << "]" << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//GenerateMatrices
// 1 vector to hold value position variables (alldiff on this).
// n*n-1 vectors, one per adjacent pair of value position variables
// 1 matrix to hold queen, primes and 0/1
// 1 display matrix
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateMatrices(int n) {
  int rowIndex, colIndex ;
  // n^2 vectors (all value position vars & adjacent pairs)
  cout << endl << n*n << endl ;
  cout << "[" ;
  for (colIndex = 0; colIndex < n*n; colIndex++) {
    cout << "x" << (indexOfFirstDiscreteVar+colIndex) ;
    if (colIndex < (n*n)-1) cout << ", " ;
  }
  cout << "]" << endl ;
  for (int valIndex = 0; valIndex < n*n-1; valIndex++)
    cout << "[x" << indexOfFirstDiscreteVar+valIndex << ", x" 
         << (indexOfFirstDiscreteVar+valIndex+1) << "]" << endl ;
  cout << endl ;

  //2 2d matrices (queen, primes, 01), displauy
  cout << 2 << endl ;
  cout << "[" ;
  for (rowIndex = 0; rowIndex < noPrimesInThisInstance; rowIndex++) {
    cout << "[x" << (indexOfFirstDiscreteVar+(n*n)) << ", "
         << "x" << (indexOfFirstDiscreteVar+primes[rowIndex]-1) << ", "
         << "x" << rowIndex << "]" ;
    if (rowIndex < noPrimesInThisInstance-1) cout << "," ;
    cout << endl ;
  }
  cout << "]" << endl ;
  // display mx
  cout << "[" ;
  cout << "[x" << indexOfFirstDiscreteVar+(n*n) << "]," << endl ;
  int varIndex = indexOfFirstDiscreteVar ;
  for (rowIndex = 0; rowIndex < n; rowIndex++) {
    cout << "[" ;
    for (colIndex = 0; colIndex < n; colIndex++) {
      cout << "x" << varIndex++ ;
      if (colIndex < (n-1)) cout << "," ;
    }
    cout << "]" ;
    if (rowIndex < (n-1)) cout << "," ;
    cout << endl ;
  } 
  cout << "]" << endl ;

  // No 3d matrices
  cout << endl << 0 << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//generateConstraints
//AllDiff on value pos
//Sum of Booleans is the bounds var
//Table ct on adjacent values
//Table ct on queens & primes
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateConstraints(int n) {
  //int rowIndex, colIndex, queenPos = 0 ;

  // sym-b for queen var
  /*cout << endl ;
  for (rowIndex = 0; rowIndex < n; rowIndex++)
    for (colIndex = 0; colIndex < n; colIndex++) {
      if ((colIndex > rowIndex) || 
          ((n%2 == 0) && (rowIndex >= n/2)) ||
	  ((n%2 != 0) && (rowIndex > n/2)))				 
        cout << "diseq(x" << indexOfFirstDiscreteVar+(n*n)
             << ", " << queenPos << ")" << endl ;
      queenPos++ ;
      }*/

  // AllDiff on value pos
  cout << endl << "alldiff(v0)" << endl ;

  // sum of Booleans is equal to bounds var.
  cout << endl ;
  cout << "sumleq(col(m0, 2), x" << indexOfFirstBoundsVar << ")" << endl ;
  cout << "sumgeq(col(m0, 2), x" << indexOfFirstBoundsVar << ")" << endl ;

  // Table ct on adjacent values
  for (int val = 1; val < n*n; val++) {
    cout << endl ;
    cout << "table( v" << val << ", t0)" << endl ;
  }

  // Table ct on queens & primes
  for (int prime = 0; prime < noPrimesInThisInstance; prime++) {
    cout << endl ;
    cout << "table( row(m0, " << prime << "), t1)" << endl ;
  }
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//generateTuples
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateTuples(int n) {
  cout << "tuplelists 2\n" << endl ;
  generateTuplesAdjacentVals(n) ;
  generateTuplesQueenPrimes(n) ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//generateTuplesAdjacentVals
//Traverse positions and calculate valid next positions
//Try all 8 moves. Move allowed if leaves you in a valid posn (0..n^2-1)
//Have to avoid wraparound moves.
//Have to generate in lex order.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateTuplesAdjacentVals(int n) {
  int newPos ;
  vector<int> adjacentTuples ;
  for (int pos = 0; pos < n*n; pos++) {
    // Left 1, Up 2.
    if ((pos % n) > 0) {                     //don't move left off board
      newPos = pos - 1 - (2*n) ;
      if (newPos >= 0) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }
    }
    // Right 1, Up 2.
    if ((pos % n) < n-1) {                 // don't move right off board
      newPos = pos + 1 - (2*n) ;
      if (newPos >= 0) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }
    }
    // Left 2, Up 1.
    if (pos % n > 1) {                       //don't move left off board
      newPos = pos - 2 - n ;
      if (newPos >= 0) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }
    }
    // Right 2, Up 1.
    if (pos % n < n-2) {
      newPos = pos + 2 - n ;
      if (newPos >= 0) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }
    }
    // Left 2, Down 1.
    if (pos % n > 1) {                       //don't move left off board
      newPos = pos - 2 + n ;
      if (newPos < n*n) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }
    }
    // Right 2, Down 1.
    if (pos % n < n-2) {
      newPos = pos + 2 + n ;
      if (newPos < n*n) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }
    }
    // Left 1, Down 2.
    if ((pos % n) > 0) {
      newPos = pos - 1 + (2*n) ;
      if (newPos < n*n) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      }      
    }
    // Right 1, Down 2.
    if ((pos % n) < n-1) {
      newPos = pos + 1 + (2*n) ;
      if (newPos < n*n) {
	    adjacentTuples.push_back(pos) ;
		adjacentTuples.push_back(newPos) ;
      } 
    }
  }
  //no of tuples, binary.
  int adjTupleIdx = 0 ;
  int adjTupleSize = adjacentTuples.size()/2 ;
  cout << adjTupleSize << " 2" << endl ;
  for (adjTupleIdx = 0; adjTupleIdx < adjTupleSize; adjTupleIdx++) {
    cout << adjacentTuples.at(2*adjTupleIdx) << " " 
		 << adjacentTuples.at(2*adjTupleIdx+1) << endl ;
  }
  cout << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//generateQueenPrimes
//Traverse queen positions and calculate positions attacked (8 dirns).
//Have to avoid wraparound moves.
//Have to generate in lex order.
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
void MinionPrimeQueenInstanceGenerator::generateTuplesQueenPrimes(int n) {
  int attackedPos, diff ;
  vector<int> queenTuples ;
  for (int queenPos = 0; queenPos < n*n; queenPos++) {
    for (attackedPos = 0; attackedPos < queenPos; attackedPos++) {
      diff = queenPos - attackedPos ;
      // is diagonally up-left?
      if (((queenPos % n) > (attackedPos % n)) &&
          (diff % (n+1) == 0)) {
	    queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
	  }
      // is straight up?
      else if (diff % n == 0) {
	    queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
      }
      // is diagonally up-right?
      else if (((queenPos % n) < (attackedPos % n)) &&
               (diff % (n-1) == 0)) {
		queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
      }
      // is left?
      else if (diff <= queenPos % n) {
		queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;
	  }
      else {
		queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(0) ;
      }
    } // end of attackedPos < queenPos loop
    for (attackedPos = queenPos+1; attackedPos < n*n; attackedPos++) {
      diff = attackedPos - queenPos ;
      // is right?
      if (diff <= attackedPos % n) {
		queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
      }
      // is diagonally down-left?
      else if (((queenPos % n) > (attackedPos % n)) &&
	       (diff % (n-1) == 0)) {
	    queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
      }
      // is down?
      else if (diff % n == 0) {
	    queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
      }
      // is down-right?
      else if (((queenPos % n) < (attackedPos % n)) &&
               (diff % (n+1) == 0)) {
	    queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(1) ;				  
      }
      else {
	    queenTuples.push_back(queenPos) ;
	    queenTuples.push_back(attackedPos) ;
	    queenTuples.push_back(0) ;
	  }
    } // end of attackedPos > queenPos loop
  } // end of queenPos loop
  int qnTupleIdx = 0 ;
  int qnTupleSize = queenTuples.size()/3 ;
  //no of tuples, ternary.
  cout << qnTupleSize << " 3" << endl ;
  for (qnTupleIdx = 0; qnTupleIdx < qnTupleSize; qnTupleIdx++) {
    cout << queenTuples.at(3*qnTupleIdx) << " " 
         << queenTuples.at(3*qnTupleIdx+1) << " "
		 << queenTuples.at(3*qnTupleIdx+2) << endl ;
  }
  cout << endl ;
}

//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
//Entrance:
// Params: v, b, r, k, l
//%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
int main(int argc, char* argv[]) {
  MinionPrimeQueenInstanceGenerator generator ;
  if (argc != 2)
    cout << "params: n" << endl ;
  else
  {
    // Check is a Minion file, and give version number of input format
	cout << "MINION 1" << endl;
    instance dummy = instance();
    cout << dummy.header();       // dummy only created to print header
    
    cout << "# Prime Queens Instance" << endl 
         << "# CSPLib prob029, www.csplib.org" << endl
         << "# Parameters: n = " << argv[1] << endl ;

    generator.generate(atoi(argv[1])) ;
  return 0 ;
  }
}
