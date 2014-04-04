x := 20;
y := 21;


perms := [];

for i in [1..(x-1)] do
    l := [1..x*y];
    for j in [1..y] do
        l[i    +(j-1)*x] := (i+1) + (j-1)*x;
        l[(i+1)+(j-1)*x] := i     + (j-1)*x;
    od;
    Append(perms, [PermList(l)]);
od;

for j in [1..(y-1)] do
    l := [1..x*y];
    for i in [1..x] do
        l[i+ j*x]    := i + (j-1)*x;
        l[i+(j-1)*x] := i + j*x;
    od;
    Append(perms, [PermList(l)]);
od;
