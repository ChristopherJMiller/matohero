// Print Ghidra's decompilation of the functions at the given addresses.
//
// Usage: analyzeHeadless ghidra BionicleHeroes -process Bionicle.exe -noanalysis -readOnly \
//          -scriptPath tools/ghidra_scripts -postScript Decompile.java 0x5654c0 [0x...]
//@category BionicleDecomp

import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

public class Decompile extends GhidraScript {
	@Override
	protected void run() throws Exception {
		DecompInterface decomp = new DecompInterface();
		decomp.openProgram(currentProgram);
		try {
			for (String arg : getScriptArgs()) {
				Function f = getFunctionContaining(toAddr(Long.decode(arg)));
				if (f == null) {
					println("no function at " + arg);
					continue;
				}
				DecompileResults res = decomp.decompileFunction(f, 60, monitor);
				// Single println so headless log prefixes don't interleave with the code.
				println(String.format("%n// %s @ 0x%s (%d bytes)%n%s", f.getName(), f.getEntryPoint(),
					f.getBody().getNumAddresses(),
					res.decompileCompleted() ? res.getDecompiledFunction().getC() : res.getErrorMessage()));
			}
		}
		finally {
			decomp.dispose();
		}
	}
}
