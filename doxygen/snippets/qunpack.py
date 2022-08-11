data = qunpack("xxTxBxZ", packet)

# returns:
# data[0], corresponds to 'T' format (QP/Spy timestamp)
# data[1], corresponds to 'B' format (standard struct.unpack())
# data[2], corresponds to 'Z' format (QP/Spy zero-terminated string)
