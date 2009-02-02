## AD 07/08
#############################################################################
##
## INPUT two literal vectors, i.e. list of integers between 1 and n^3
##
## OUTPUT two shortened literal vectors where the least significant pair
##        of every cycle is removed
##
## METHOD the two vectors represent bit-vectors and these will be ordered 
##        using lex. For example ABC compared with CAB. Here the third
##        position is least significant and will thus never decide about the
##        ordering of the two vectors. A pair of literals will be removed 
##        if everything other pair of the same cycle appeared before.
##

rule1 := function( v1, v2 )
    local i, newv1, newv2, pos, elm;

    newv1 := [ ];
    newv2 := [ ];

    for i in [ 1..Length( v1 ) ] do
        if v1[i] <> v2[i] then
            elm := v2[i];
            pos := Position( newv1, elm );
            while pos <> fail do
                elm := newv2[pos];
                pos := Position( newv1, elm );
            od;
            if elm <> v1[i] then
                Add( newv1, v1[i] );
                Add( newv2, v2[i] );
            fi;
        fi;
    od;

    return [ newv1, newv2 ];
end;

gen_constraints := function(list, varnames, a)
  local perm, i, j, vecs, set;

  set := [];
  for i in list do
    perm := ListPerm(i);
    perm := perm{[1..Minimum(Size(perm), Size(varnames))]};
    vecs := rule1([1..Size(perm)], perm);
    Append(set, [vecs]);
  od;

  set := Set(set);

  for vecs in set do
    PrintTo(a, "lexleq( [");
    for j in vecs[1] do
      PrintTo(a, varnames[j], ",");
    od;
    PrintTo(a, "], [");
    for j in vecs[2] do
      PrintTo(a, varnames[j], ",");
    od;
    PrintTo(a, "] )\n");
  od;
end;
