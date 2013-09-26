
struct Backtrackable
{
    virtual void mark() {
        cout << "Call to mark on class that does not implement it." <<endl;
        abort();
    }
    
    virtual void pop() {
        cout << "Call to pop on class that does not implement it." <<endl;
        abort();
    }
};




struct GenericBacktracker 
{
private:
    vector<Backtrackable*> things;
    GenericBacktracker(const GenericBacktracker& );
    
public:
    GenericBacktracker() {
    }
    
    void world_pop() {
        int size=things.size();
        for(int i=0; i<size; i++) {
            things[i]->pop();
        }
    }
    
    void mark() {
        int size=things.size();
        for(int i=0; i<size; i++) {
            things[i]->mark();
        }
    }
    
    void add(Backtrackable* bt_obj) {
        things.push_back(bt_obj);
        
    }
    
    
};
