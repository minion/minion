#
# Search node data:
#
#  selected -- indices in set of points being mapped to minimal image at this node
#  image    -- sequence-wise image of set under element represented by this node
#  substab --  Stab_K(selected) sequence stabilizer
#  children --  nodes corresponding to extensions of selected
#  parent -- node corr to all but last element of selected.]
#  childno 
#  next -- across row
#  prev -- likewise
#  deleted 
#

# At each level

# Find the next pt of minimum image and all the corresponding nodes

# If at any time in this process we notice a potential prune, we have a
# generator of K -- construct it, close K with it and delete all tree
# branches that are now non-canonical -- propagate new K down through
# tree. Continue with surviving nodes at current level.

HASH_LIMIT :=100;


TIME_CLASSES := [];

DeclareTimeClass := function(name)
    BindGlobal(name, Length(TIME_CLASSES)+1);
    Add(TIME_CLASSES,name);
end;


DeclareTimeClass("pass1");
DeclareTimeClass("pass2");
DeclareTimeClass("pass3");
DeclareTimeClass("shortcut");
DeclareTimeClass("changeStabChain");
DeclareTimeClass("orbit");
DeclareTimeClass("skippedorbit");
DeclareTimeClass("getcands");
DeclareTimeClass("improve");
DeclareTimeClass("check1");
DeclareTimeClass("check2");
DeclareTimeClass("prune");


stats := fail;

DO_TIMING := true;

if DO_TIMING then
    StartTimer := function(cat)
        stats[cat] := stats[cat] - Runtime();
    end;

    StopTimer := function(cat)
        stats[cat] := stats[cat] + Runtime();
    end;

    IncCount := function(cat)
        stats[cat] := stats[cat] + 1;
    end;

    ResetStats := function()
        stats := ListWithIdenticalEntries(Length(TIME_CLASSES),0);
    end;

    ResetStats();

    GetStats := function()
        local   r,  c;
        r := rec();
        for c in TIME_CLASSES do
            r.(c) := stats[ValueGlobal(c)];
        od;
        return r;
    end;

else
    StartTimer := function(cat)
        return;
    end;

    StopTimer := function(cat)
        return;
    end;

    IncCount := function(cat)
        return;
    end;

    ResetStats := function()
        return;
    end;

    GetStats := function()
        return fail;
    end;

fi;



if not IsBound(InfoNSI) then
    DeclareInfoClass("InfoNSI");
fi;


NewSmallestImage := function(g,set,k,skip_func)
    local   leftmost_node,  next_node,  delete_node,  delete_nodes,  
            clean_subtree,  handle_new_stabilizer_element,  
            simpleOrbitReps,  make_orbit,  n,  s,  l,  m,  hash,  
            lastupb,  root,  depth,  gens,  orbnums,  orbmins,  
            orbsizes,  upb,  imsets,  imsetnodes,  node,  cands,  y,  
            x,  num,  rep,  node2,  prevnode,  nodect,  changed,  
            newnode,  image,  dict,  seen,  he,  bestim,  bestnode,  
            imset,  p;
    leftmost_node := function(depth)
        local   n,  i;
        n := root;
        while Length(n.selected) < depth -1 do
            n := n.children[1];
        od;
        return n;
    end;
    next_node := function(node)
        local   n;
        n := node;
        repeat
            n := n.next;
        until n = fail or not n.deleted;
        return n;
    end;
    delete_node := function(node)
        local   i;
        if node.deleted then 
            return;
        fi;
        Info(InfoNSI,3,"Deleting ",node.selected);
        if node.prev <> fail then
            node.prev.next := node.next;
        fi;
        if node.next <> fail then
            node.next.prev := node.prev;
        fi;
        node.deleted := true;
        if node.parent <> fail then
            Remove(node.parent.children, node.childno);
            if Length(node.parent.children) = 0 then
                delete_node(node.parent);
            else
                for i in [node.childno..Length(node.parent.children)] do
                    node.parent.children[i].childno := i;
                od;
            fi;
        fi;
        if IsBound(node.children) then
            delete_nodes(ShallowCopy(node.children));
        fi;
    end;
    delete_nodes := function(nodes)
        local   node;
        for node in nodes do
            delete_node(node);
        od;
    end;
    
    clean_subtree := function(node)
        local   bad,  seen,  c,  x,  q,  gens,  olen,  pt,  gen,  im;
        Info(InfoNSI,3,"Cleaning at ",node.selected);
        if not IsBound(node.children) then
            return;
        fi;
        bad := [];
        
        seen := BlistList([1..m],[]);
        for c in node.children do
            x := c.selected[Length(c.selected)];
            if seen[x] then
                Add(bad,c);
            else
                q := [x];
                gens := GeneratorsOfGroup(node.substab);
                olen := 1;
                seen[x] := true;
                for pt in q do
                    for gen in gens do
                        im := pt^gen;
                        if not seen[im] then
                            seen[im] := true;
                            Add(q,im);
                            olen := olen+1;
                        fi;
                    od;
                od;
                if olen < Size(node.substab)/Size(c.substab) then
                    c.substab := Stabilizer(node.substab,x);
                    clean_subtree(c);
                fi;
            fi;
        od;
        delete_nodes(bad);
    end;

    handle_new_stabilizer_element := function(node1,node2)
        local   perm1,  i;
        # so node1 and node2 represnet group elements that map set to the same 
        # place in two different ways
        perm1 := PermListList(node1.image, node2.image);
        Assert(1, not perm1 in l);
        l := ClosureGroup(l,perm1);
        Info(InfoNSI,2,"Found new stabilizer element. Stab now ",Size(l));        
        root.substab := l;
        clean_subtree(root);
    end;
    simpleOrbitReps := function(gp,set)
        local   m,  n,  b,  seed,  reps,  gens,  q,  pt,  gen,  im;
        m := Length(set);
        n := set[m];
        b := BlistList([1..n],set);
        seed := set[1];
        reps := [];
        gens := GeneratorsOfGroup(gp);
        while seed <> fail and seed <= n do
            b[seed] := false;
            q := [seed];
            Add(reps,seed);
            for pt in q do
                for gen in gens do
                    im := pt^gen;
                    if b[im] then
                        b[im] := false;
                        Add(q,im);
                    fi;
                od;
            od;
            seed := Position(b,true,seed);
        od;
        return reps;
    end;
    make_orbit := function(x)
        local   q,  rep,  num,  pt,  gen,  img;
        q := [x];
        rep := x;
        num := Length(orbmins)+1;
        orbnums[x] := num;
        for pt in q do
            for gen in gens do
                img := pt^gen;
                if orbnums[img] = -1 then
                    orbnums[img] := num;
                    Add(q,img);
                    if img < rep then
                        rep := img;
                    fi;
                fi;
            od;
        od;
        Add(orbmins,rep);
        Add(orbsizes,Length(q));
        return num;
    end;
    
    if set = [] then
      return [ [], k];
    fi;

    n := Maximum(LargestMovedPoint(g), Maximum(set));
    s := StabChainMutable(g);
    l := Action(k,set);
    m := Length(set);
    hash := s->JENKINS_HASH(s,GAPInfo.BytesPerVariable*m+GAPInfo.BytesPerVariable);
    lastupb := 0;
    root := rec(selected := [], 
                image := set,
                imset := Immutable(Set(set)),
                substab := l,
                deleted := false,
                next := fail,
                prev := fail,
                parent := fail);
    for depth in [1..m] do
        gens := s.generators;
        orbnums := ListWithIdenticalEntries(n,-1);
        orbmins := [];
        orbsizes := [];
        upb := infinity;
        imsets := [];
        imsetnodes := [];
        #
        # At this point, all bottom nodes are blue
        # first pass creates appropriate set of virtual red nodes
        #
        StartTimer(pass1);
        node := leftmost_node(depth);
        while node <> fail do
            StartTimer(getcands);
            cands := Difference([1..m],skip_func(node.selected));
            if Length(cands) > 1 and not IsTrivial(node.substab) then
                cands := simpleOrbitReps(node.substab,cands);
            fi;
            #
            # These index the children of node that will
            # not be immediately deleted under rule C
            #
            StopTimer(getcands);
            node.validkids := [];
            for y in cands do
                IncCount(check1);
                x := node.image[y];
                num := orbnums[x];
                if num = -1 then
                    #
                    # Need a new orbit. Also require the smallest point
                    # as the rep.
                    #
                    #
                    # If there is no prospect of the new orbit being 
                    # better than the current best then go on to the next candidate
                    #
                    if upb <= lastupb+1 then
                        IncCount(skippedorbit);
                        continue;
                    fi;
                    StartTimer(orbit);
                    num := make_orbit(x);
                    StopTimer(orbit);
                    rep := orbmins[num];
                    if rep < upb then
                        StartTimer(improve);
                        upb := rep;
                        node2 := node.prev;
                        while node2 <> fail do
                            delete_node(node2);
                            node2 := node2.prev;
                        od;
                        node.validkids := [y];
                        Info(InfoNSI,3,"Best down to ",upb);
                        StopTimer(improve);
                    fi;
                else
                    IncCount(check2);
                    rep := orbmins[num];
                    if rep = upb then
                        Add(node.validkids,y);
                    fi;
                fi;
            od;
            if node.validkids = [] then
                StartTimer(prune);
                delete_node(node);
                StopTimer(prune);
            fi;
            node := next_node(node);
        od;
        Info(InfoNSI,2,"Layer ",depth," pass 1 complete. Best is ",upb);
        StopTimer(pass1);
        #
        # Second pass. Actually make all the red nodes and turn them blue
        #
        lastupb := upb;
        StartTimer(changeStabChain);
        ChangeStabChain(s,[upb],false);
        StopTimer(changeStabChain);
        if Length(s.orbit) = 1 then
            #
            # In this case nothing much can happen. Each surviving node will have exactly one child
            # and none of the imsets will change
            # so we mutate the nodes in-place 
            #
            StartTimer(shortcut);
            node := leftmost_node(depth);
            while node <> fail do
                Add(node.selected, node.validkids[1]);
                node := next_node(node);
            od;
            Info(InfoNSI,2,"Nothing can happen, short-cutting");
            s := s.stabilizer;
            StopTimer(shortcut);
            if Size(skip_func(leftmost_node(depth+1).selected)) = m then
                Info(InfoNSI,2,"Skip would skip all remaining points");
                break;
            fi;

            continue; # to the next depth
        fi;
        StartTimer(pass2);
        node := leftmost_node(depth);
        prevnode := fail;
        nodect := 0;
        changed := false;
        while node <> fail do
            node.children := [];
            for x in node.validkids do
                newnode := rec( selected := Concatenation(node.selected,[x]),
                                substab := Stabilizer(node.substab,x),
                                parent := node,
                                childno := Length(node.children)+1,
                                next := fail,
                                prev := prevnode,
                                deleted := false);
                nodect := nodect+1;
                if prevnode <> fail then
                    prevnode.next := newnode;
                fi;
                prevnode := newnode;
                Add(node.children,newnode);
                image := node.image;
                if image[x] <> upb then
                    repeat
                        image := OnTuples(image, s.transversal[image[x]]);
                    until image[x] = upb;
                    newnode.image := image;
                    newnode.imset := Set(image);
                    MakeImmutable(newnode.imset);
                    changed := true;
                else
                    newnode.image := image;
                    newnode.imset := node.imset;
                fi;
#                Print("Made a node ",newnode.selected, " ",newnode.image,"\n");
            od;
            node := next_node(node);
        od;
        StopTimer(pass2);
        Info(InfoNSI,2,"Layer ",depth," pass 2 complete. ",nodect," new nodes");
        #
        # Third pass detect stabilizer elements
        #
        
        StartTimer(pass3);
        if  changed then
            node := leftmost_node(depth+1);
            if nodect > HASH_LIMIT then
                dict := SparseHashTable(hash);
                seen := [];
                while node <> fail do 
                    he := GetHashEntry(dict,node.imset);
                    if  fail <> he then
                        handle_new_stabilizer_element(node, he);
                    else
                        AddHashEntry(dict, node.imset, node);
#                    if hash(node.imset) in seen then
#                        Error("");
#                    fi;
#                    AddSet(seen, hash(node.imset));
                    fi;
                    node := next_node(node);
                od;
                Info(InfoNSI,2,"Layer ",depth," pass 3 complete. Used hash table");
                s := s.stabilizer;
                if Length(s.generators) = 0 then
                    Info(InfoNSI,2,"Run out of group, return best image");
                    node := leftmost_node(depth+1);
                    bestim := node.imset;
                    bestnode := node;
                    node := next_node(node);
                    while node <> fail do
                        if node.imset < bestim then
                            bestim := node.imset;
                            bestnode := node;
                        fi;
                        node := next_node(node);
                    od;
                    StopTimer(pass3);
                    return [bestnode.image,l];
                fi;
            else
                while node <> fail do
                    imset := node.imset;
                    p := PositionSorted(imsets, imset);
                    if p <= Length(imsets) and imsets[p] = imset then
                        handle_new_stabilizer_element(node, imsetnodes[p]);
                    else
                        Add(imsets,imset,p);
                        Add(imsetnodes,node,p);
                    fi;
                    node := next_node(node);
                od;
                Info(InfoNSI,2,"Layer ",depth," pass 3 complete. ",Length(imsets)," images");
                s := s.stabilizer;
                if Length(s.generators) = 0 then
                    Info(InfoNSI,2,"Run out of group, return best image");
                    StopTimer(pass3);
                    return [imsetnodes[1].image,l];
                fi;
            fi;
        else
            s := s.stabilizer;
        fi;
        StopTimer(pass3);
        if Size(skip_func(leftmost_node(depth+1).selected)) = m then
            Info(InfoNSI,2,"Skip would skip all remaining points");
            break;
        fi;
    od;                
    return [leftmost_node(depth+1).image,l];
end;
                               
        
        

        
