import copy

def initialize_domain(domain):
	global var_start
	global domain_store
	global total_litcount
	domain_store = copy.deepcopy(domain)
	var_start = [1]
	for d in domain[:-1]:
		var_start.append(var_start[-1] + len(d))
	total_litcount = var_start[-1] + len(domain[-1]) - 1

def get_lit(var, val):
	return var_start[var] + domain_store[var].index(val)

def get_total_varcount():
	return len(domain_store)

def get_domain(var):
	return domain_store[var]

def get_total_litcount():
	return total_litcount

def get_lit_mapping():
	lit_map = []
	for i in range(len(domain_store)):
		for d in domain_store[i]:
			lit_map.append([i,d])
	return lit_map
