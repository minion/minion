import xml.etree.ElementTree as et
import socket
import types

def OMS_node(cd, name):
    OMSnode = et.Element("OMS")
    OMSnode.attrib = { "cd" : cd, "name" : name }
    return OMSnode

def OMATP_node():
    # Note : I don't know what any of this means
    OMATPnode = et.Element("OMATP")
    OMATPnode.append(OMS_node("scscp1", "call_id"))
    OMSTR = et.Element("OMSTR")
    OMSTR.text = "localhost:1:1:432"
    OMATPnode.append(OMSTR)
    OMATPnode.append(OMS_node("scscp1", "option_return_object"))
    OMATPnode.append(et.Element("OMSTR"))
    return OMATPnode

def int_node(i):
    node = et.Element("OMI")
    node.text = str(i)
    return node

def permutation_node(list):
    base = et.Element("OMA")
    base.append(OMS_node("permut1", "permutation"))
    for l in list:
         base.append(int_node(l))
    return base

def group_node(perm_list):
    base = et.Element("OMA")
    base.append(OMS_node("permgp1", "group"))
    base.append(OMS_node("permutation1", "right_compose"))
    for p in perm_list:
        base.append(permutation_node(p))
    return base

def listint_node(arg):
    if isinstance(arg, types.ListType):
        base = et.Element("OMA")
        base.append(OMS_node("list1", "list"))
        for a in arg:
            base.append(listint_node(a))
        return base
    if isinstance(arg, types.IntType) or isinstance(arg, types.LongType):
        base = et.Element("OMI")
        base.text = str(arg)
        return base
    assert 0, "Do not understand " + str(arg)

def read_listint(arg):
    if arg.tag == "OMI":
        return int(arg.text)
    if arg.tag == "OMA":
        assert ( arg[0].tag == "OMS" and
                           ( arg[0].attrib == { "cd":"list1","name":"list" } or
                             arg[0].attrib == { "cd":"linalg2", "name":"matrix" } or
                             arg[0].attrib == { "cd":"linalg2", "name":"matrixrow" } or
                             arg[0].attrib == { "cd":"set1", "name":"set" } ) )
        list = []
        for i in arg[1:]:
          list += [read_listint(i)]
        return list
    if arg.tag == "OMS":
        assert arg.attrib == { "cd":"set1", "name":"emptyset" }
        return []
    assert 0, "Do not understand " + str(arg)

def parse_reply(arg, check_cd = True):
    return read_listint(arg[0][1][1])

def build_call(proc_name, args) :
    OBJbase = et.Element("OMOBJ")
    OBJbase.append(et.Element("OMATTR"))
    OBJbase[0].append(OMATP_node())
    base = et.Element("OMA")
    base.append(OMS_node("scscp1", "procedure_call"))
    OMAnode = et.Element("OMA")
    OMAnode.append(OMS_node("scscp_transient_1", proc_name))
    for a in args:
      OMAnode.append(a)
    base.append(OMAnode)
    OBJbase[0].append(base)
    return OBJbase

def read_reply(socket) :
    file = socket.makefile()
    line = str(file.readline()).strip()
    while(line != "<?scscp start ?>") :
        line = str(file.readline()).strip()

    line = str(file.readline()).strip()
    outline = ""
    while(line != "<?scscp end ?>") :
        outline += line + '\n';
        line = str(file.readline()).strip()
    return outline

def make_connection():
    scscp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    scscp_socket.connect(("localhost", 26133))
    opening_string = scscp_socket.makefile().readline()
    scscp_socket.sendall('<?scscp scscp_version="1.3" ?>\n')
    opening_string = scscp_socket.makefile().readline()
    return scscp_socket

def execute(socket, proc):
    command = '\n<?scscp start ?>\n' + et.tostring(proc) + '\n<?scscp end ?>\n'
    socket.sendall(command)
    return et.fromstring(read_reply(socket))

if __name__ == '__main__':
  scscp_socket = make_connection()
  group = group_node([[1,3,2],[4,2,3,1]])
  command = build_call("WS_Size",[listint_node([0,0,0,0,0]) ] )
  print et.dump(command)
  reply = execute(scscp_socket, command)
  et.dump(reply)
  print(parse_reply(reply))

