MapListList := function(L,p)
  local x, y;
  return List(L, x -> (List(x, y -> y^p)));
end;
  
NewSmallestImageList := function(G, L)
  local stab, x, image, mapper, conj, perm, Limage;
  if L = [] then
    return [()];
  fi;
  stab   := Stabilizer(G, L[1], OnSets);
  image  := NewSmallestImage(G, L[1], stab, x -> x);
  mapper := RepresentativeAction(G, L[1], image[1], OnSets);
  conj   := ConjugateGroup(stab, mapper);
  Limage := List(L{[2..Length(L)]}, x -> (List(x, y -> y^mapper)));
  perm   := NewSmallestImageList(conj, Limage);
  return [ mapper*perm[1], stab, image, mapper, conj, Limage, perm, MapListList(L, mapper*perm[1])];
end;

