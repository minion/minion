
var minion =
    { run: function(model, opts) {
        if (model == undefined) {
            console.log("minion-js error: No model provided.");
            return;
        }
        var args   = (opts != undefined && opts.args   != undefined) ? opts.args   : [];
        var stdout = (opts != undefined && opts.stdout != undefined) ? opts.stdout : function(text) { console.log("stdout: " + text); };
        var stderr = (opts != undefined && opts.stderr != undefined) ? opts.stderr : function(text) { console.log("stderr: " + text); };
        var Module =
            { "arguments": args.concat(["-printsolsonly", "input.minion"])
            , "print"    : stdout
            , "printErr" : stderr
            , "preRun"   : function() {
                    FS.writeFile("input.minion", model);
                }
            };
