// Export a function list and a source-file hint map from the analyzed program.
//
// Writes into the directory given as the first script argument (default: build/):
//   functions.csv     address,size,name,thunk,callers,callees
//   source_hints.csv  source_path,function_address,function_name,string_address
//
// Source hints come from strings like ".\pc\nurndr.c" (assert/debug file names): every
// function referencing one is tagged with that path. MSVC emits each object's functions
// contiguously, so the hints sketch the original object file boundaries.
//
// Usage: analyzeHeadless ghidra BionicleHeroes -process Bionicle.exe -noanalysis \
//          -scriptPath tools/ghidra_scripts -postScript ExportOverview.java build
//@category BionicleDecomp

import java.io.File;
import java.io.PrintWriter;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Data;
import ghidra.program.model.listing.DataIterator;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.symbol.Reference;

public class ExportOverview extends GhidraScript {
	private static final Pattern SOURCE = Pattern.compile("^\\.?\\\\[\\w\\\\./-]+\\.(c|cpp|h|hpp)$", Pattern.CASE_INSENSITIVE);

	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		File outDir = new File(args.length > 0 ? args[0] : "build");
		outDir.mkdirs();

		int total = 0, named = 0;
		try (PrintWriter w = new PrintWriter(new File(outDir, "functions.csv"))) {
			w.println("address,size,name,thunk,callers,callees");
			FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
			while (it.hasNext() && !monitor.isCancelled()) {
				Function f = it.next();
				total++;
				if (!f.getName().startsWith("FUN_")) {
					named++;
				}
				w.printf("0x%s,%d,%s,%b,%d,%d%n", f.getEntryPoint(), f.getBody().getNumAddresses(),
					f.getName(true), f.isThunk(), f.getCallingFunctions(monitor).size(),
					f.getCalledFunctions(monitor).size());
			}
		}

		int hints = 0;
		try (PrintWriter w = new PrintWriter(new File(outDir, "source_hints.csv"))) {
			w.println("source_path,function_address,function_name,string_address");
			DataIterator it = currentProgram.getListing().getDefinedData(true);
			while (it.hasNext() && !monitor.isCancelled()) {
				Data d = it.next();
				if (!d.hasStringValue()) {
					continue;
				}
				String s = String.valueOf(d.getValue()).trim();
				if (!SOURCE.matcher(s).matches()) {
					continue;
				}
				for (Reference ref : getReferencesTo(d.getAddress())) {
					Function f = getFunctionContaining(ref.getFromAddress());
					if (f == null) {
						continue;
					}
					hints++;
					w.printf("%s,0x%s,%s,0x%s%n", s, f.getEntryPoint(), f.getName(true), d.getAddress());
				}
			}
		}

		println(String.format("functions: %d (%d named by analysis), source hints: %d", total, named, hints));
	}
}
