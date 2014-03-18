LoadPackage("grape");

MapListSet := function(L,p)
  local x, y;
  return List(L, x -> List(Set(x, y -> y^p)) );
end;
  
NewSmallestImageList := function(G, L)
  local stab, x, image, mapper, conj, perm, Limage;
  if L = [] then
    return [()];
  fi;
  #Print(L);
  #Print("Before");
  stab   := Stabilizer(G, Set(L[1]), OnSets);
  #Print("After");
  image  := NewSmallestImage(G, L[1], stab, x -> x);
  mapper := RepresentativeAction(G, L[1], image[1], OnTuples);
  conj   := ConjugateGroup(stab, mapper);
  Limage := List(L{[2..Length(L)]}, x -> (List(x, y -> y^mapper)));
  perm   := NewSmallestImageList(conj, Limage);
  return [ mapper*perm[1], stab, image, mapper, conj, Limage, perm];
end;


CAJ_MinListImage := function(perms, L)
  local g, ret;
  g := GroupByGenerators(List(perms, PermList),());
  ret := NewSmallestImageList(g, L);
  return [ListPerm(ret[1]), MapListSet(L, ret[1])];

end;

CAJ_GroupSizeImpl := function(perms)
  return Size(GroupByGenerators(List(perms, PermList),()));
end;

CAJ_GetGraphGens := function(tuples, maxpoint)
  local graphsize,graph, i, j, grapegraph,g;
  
  graphsize := maxpoint + Size(tuples);
  graph := List([1..graphsize], x -> List([1..graphsize], y -> 0));
  for i in [1..Size(tuples)] do
    for j in tuples[i] do
      graph[i + maxpoint][j] := 1;
      graph[j][i + maxpoint] := 1;
    od;
  od;


  grapegraph := Graph( Group(()),  [1..graphsize], OnPoints, function(x,y) return graph[x][y]=1; end, true );
  g := AutGroupGraph( rec(graph:=grapegraph, colourClasses := [[1..maxpoint],[maxpoint+1..graphsize]]));

  if g = Group(()) then
    g := Group(());
  fi;
  
  return List(GeneratorsOfGroup(g), x->ListPerm(x, maxpoint));
end; 
