
var minion =
    { run: function(model, opts) {
        if (model == undefined) {
            console.log("minion-js error: No model provided.");
            return;
        }
        var args          = (opts != undefined && opts.args          != undefined) ? opts.args          : [];
        var stdoutHandler = (opts != undefined && opts.stdoutHandler != undefined) ? opts.stdoutHandler : function(text) { console.log("stdout: " + text); };
        var stderrHandler = (opts != undefined && opts.stderrHandler != undefined) ? opts.stderrHandler : function(text) { console.log("stderr: " + text); };
        var statsHandler  = (opts != undefined && opts.statsHandler  != undefined) ? opts.statsHandler  : function(text) { console.log("stats: "  + text); };
        var Module =
            { "arguments": args.concat([ "input.minion"
                                       , "-printsolsonly"
                                       , "-tableout", "stats.txt"
                                       ])
            , "print"    : stdoutHandler
            , "printErr" : stderrHandler
            , "preRun"   : function() {
                    FS.writeFile("input.minion", model, {encoding: "utf8"});
                }
            , "postRun"  : function() {
                    statsHandler(FS.readFile("stats.txt", {encoding: "utf8"}));
                }
            };
