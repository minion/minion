template<typename T, size_t s>
bool increment_array(array<T, s>& in, int limit)
{
    for(size_t i = 0; i < in.size(); ++i)
    {
        int val = in[i];
        val++;
        assert(val <= limit);
        if(val == limit)
            in[i] = 0;
        else
        {
            in[i] = val;
            return true;
        }
    }
    return false;
}

bool assignment_inDomains(const array<int, vcount> assign, const InData& id)
{
    for(int i = 0; i < vcount; ++i)
    {
        if(!id[i*domsize + assign[i]])
            return false;
    }
    return true;
}

template<typename C>
OutData prune_domains(const InData& id,C c)
{
    OutData od;
    for(size_t i = 0; i < id.size(); ++i)
    {
        int var = i / domsize;
        int dom = i - var * domsize;

        if(id[i] == false)
        {
            od[i] = TM;
        }
        else
        {
            // Need to check if id[i] has support.
            array<int, vcount> assign;
            for(int l = 0; l < vcount; ++l) assign[l] = 0;
            bool supported = false;
            do
            {
                if(assign[var] == dom && c(assign) && assignment_inDomains(assign, id))
                    supported = true;
            }
            while(increment_array(assign, domsize));
            
            if(supported)
              od[i] = TT;
            else
              od[i] = TF;
        }

    }
    return od;
}

