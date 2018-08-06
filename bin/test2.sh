#minion -varorder domoverwdeg -noprintsols -timelimit 10 sonet-60-45-33-2768.eprime-param.minion

inst='sonet-60-45-33-2768.eprime-param.minion'
#inst='test.minion'

config=''

# counter
#config="${config} -nhconfig counttime 500"
config="${config} -nhconfig countbacktracks 2.0 1.1 0 1.1 onfailure"

# search algorithm
#config="${config} -nhsearch metawithhillclimbing 4 0.001 0.0625"
#config="${config} -nhsearch metawithlahc 100 1.0"
config="${config} -nhsearch sa 0.8 5 0.995 100"

# neighbourhood selection strategy
#config="${config} -nhselection random"
config="${config} -nhselection ucb 2.0"
#config="${config} -nhselection la 0.1"

timelimit=300
seed=123
echo ${config}
minion -varorder domoverwdeg -noprintsols -timelimit ${timelimit} -randomseed ${seed} ${config} ${inst} 
