// Ghidra post-script: decompile the functions at the given hex addresses (or all functions if the
// first argument is "all") into <outdir>/<address>_<name>.c
import java.io.File;
import java.io.FileWriter;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;

public class DecompileFunctions extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 2) throw new IllegalArgumentException("usage: DecompileFunctions.java <outdir> <all|addr...>");
        File out = new File(args[0]);
        out.mkdirs();

        List<Function> fns = new ArrayList<>();
        if (args[1].equals("all")) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            while (it.hasNext()) fns.add(it.next());
        } else {
            for (int i = 1; i < args.length; i++) {
                Function f = getFunctionAt(toAddr(Long.parseLong(args[i], 16)));
                if (f == null) println("no function at " + args[i]); else fns.add(f);
            }
        }

        DecompInterface ifc = new DecompInterface();
        ifc.openProgram(currentProgram);
        int ok = 0;
        for (Function f : fns) {
            DecompileResults res = ifc.decompileFunction(f, 60, monitor);
            String name = String.format("%08x_%s.c", f.getEntryPoint().getOffset(), f.getName());
            try (PrintWriter w = new PrintWriter(new FileWriter(new File(out, name)))) {
                if (res.decompileCompleted()) {
                    w.print(res.getDecompiledFunction().getC());
                    ok++;
                } else {
                    w.println("// decompile failed: " + res.getErrorMessage());
                }
            }
        }
        ifc.dispose();
        println("decompiled " + ok + "/" + fns.size() + " functions into " + out);
    }
}
