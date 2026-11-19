import sys
out = sys.argv[1]
db  = ('-30', '0', '0')  # min, max, default
plugins = [
  ("LoudMax Mono", "lv2:CompressorPlugin",
   [("ctl","threshold","Threshold (dB)",db),("ctl","output_gain","Output (dB)",db),
    ("tog","isp","Inter-Sample Peaks"),("lat",),
    ("ain","in","Input"),("aout","out","Output")]),
  ("LoudMax Stereo", "lv2:CompressorPlugin",
   [("ctl","threshold","Threshold (dB)",db),("ctl","output_gain","Output (dB)",db),
    ("tog","isp","Inter-Sample Peaks"),("lat",),
    ("ain","in_l","Input (Left)"),("aout","out_l","Output (Left)"),
    ("ain","in_r","Input (Right)"),("aout","out_r","Output (Right)")]),
  ("LoudMax 5.1", "lv2:CompressorPlugin",
   [("ctl","thr_quad","Threshold Quad (dB)",db),("ctl","out_quad","Output Quad (dB)",db),
    ("ctl","thr_c","Threshold Center (dB)",db),("ctl","out_c","Output Center (dB)",db),
    ("ctl","thr_lfe","Threshold LFE (dB)",db),("ctl","out_lfe","Output LFE (dB)",db),
    ("link",),("tog","isp","Inter-Sample Peaks"),("lat",)] +
   sum([[("ain","in_"+s,"Input (%s)"%n),("aout","out_"+s,"Output (%s)"%n)] for s,n in
        [("fl","Front Left"),("fr","Front Right"),("c","Center"),("lfe","LFE"),("bl","Back Left"),("br","Back Right")]],[])),
]
pre = """@prefix lv2:  <http://lv2plug.in/ns/lv2core#> .
@prefix doap: <http://usefulinc.com/ns/doap#> .
@prefix foaf: <http://xmlns.com/foaf/0.1/> .
@prefix rdfs: <http://www.w3.org/2000/01/rdf-schema#> .
@prefix units: <http://lv2plug.in/ns/extensions/units#> .
"""
man = pre
ttl = pre
for i,(name,cls,ports) in enumerate(plugins):
    uri = "<urn:ladspa-wrap:loudmax:%d>" % i
    man += "\n%s a lv2:Plugin ;\n  lv2:binary <loudmax_lv2.so> ;\n  rdfs:seeAlso <loudmax.ttl> .\n" % uri
    ps = []
    for idx,p in enumerate(ports):
        k = p[0]
        if k == "ctl":
            ps.append('a lv2:InputPort, lv2:ControlPort ; lv2:index %d ; lv2:symbol "%s" ; lv2:name "%s" ; lv2:minimum %s ; lv2:maximum %s ; lv2:default %s ; units:unit units:db' % (idx,p[1],p[2],*p[3]))
        elif k == "tog":
            ps.append('a lv2:InputPort, lv2:ControlPort ; lv2:index %d ; lv2:symbol "%s" ; lv2:name "%s" ; lv2:minimum 0 ; lv2:maximum 1 ; lv2:default 0 ; lv2:portProperty lv2:toggled' % (idx,p[1],p[2]))
        elif k == "link":
            ps.append('a lv2:InputPort, lv2:ControlPort ; lv2:index %d ; lv2:symbol "link_mode" ; lv2:name "Link Mode" ; lv2:minimum 0 ; lv2:maximum 3 ; lv2:default 1 ; lv2:portProperty lv2:integer, lv2:enumeration ;\n    lv2:scalePoint [ rdfs:label "Link all" ; rdf:value 0 ], [ rdfs:label "5-Link" ; rdf:value 1 ], [ rdfs:label "Quad+C Link" ; rdf:value 2 ], [ rdfs:label "Quad Link" ; rdf:value 3 ]' % idx)
        elif k == "lat":
            ps.append('a lv2:OutputPort, lv2:ControlPort ; lv2:index %d ; lv2:symbol "latency" ; lv2:name "Latency" ; lv2:minimum 0 ; lv2:maximum 192000 ; lv2:portProperty lv2:reportsLatency, lv2:integer ; lv2:designation lv2:latency ; units:unit units:frame' % idx)
        elif k == "ain":
            ps.append('a lv2:InputPort, lv2:AudioPort ; lv2:index %d ; lv2:symbol "%s" ; lv2:name "%s"' % (idx,p[1],p[2]))
        elif k == "aout":
            ps.append('a lv2:OutputPort, lv2:AudioPort ; lv2:index %d ; lv2:symbol "%s" ; lv2:name "%s"' % (idx,p[1],p[2]))
    ttl += "\n%s a lv2:Plugin, %s ;\n  doap:name \"%s\" ;\n  doap:maintainer [ foaf:name \"Thomas Mundt\" ] ;\n  lv2:optionalFeature lv2:hardRTCapable ;\n  lv2:port\n  [ %s ] .\n" % (uri, cls, name, " ] ,\n  [ ".join(ps))
ttl = ttl.replace("@prefix units:", "@prefix rdf: <http://www.w3.org/1999/02/22-rdf-syntax-ns#> .\n@prefix units:",1)
open(out+"/manifest.ttl","w").write(man)
open(out+"/loudmax.ttl","w").write(ttl)
