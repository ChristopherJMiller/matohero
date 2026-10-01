// Name PC functions from the PS2 prototype mapping (tools/ps2names.py).
//
// Reads orig/ps2proto/pc_names.csv (or the path given as the first script argument),
// renames each function (C++ "Class::method" goes into a class namespace) and sets a
// plate comment with the PS2 signature, object file and how the pairing was found.
// Functions already given a non-default name by a user are left alone.
//
// Usage: analyzeHeadless ghidra BionicleHeroes -process Bionicle.exe -noanalysis \
//          -scriptPath tools/ghidra_scripts -postScript ApplyPs2Names.java
//@category BionicleDecomp

import java.io.BufferedReader;
import java.io.FileReader;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import ghidra.app.script.GhidraScript;
import ghidra.app.util.NamespaceUtils;
import ghidra.program.model.listing.CodeUnit;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.Namespace;
import ghidra.program.model.symbol.SourceType;

public class ApplyPs2Names extends GhidraScript {
	// GCC 2.95 mangling, when the map had no demangled form: name__F<params>, name__<class>
	private static final Pattern MANGLED = Pattern.compile("^(\\w+?)__(F|\\d|C|Q).*$");

	@Override
	protected void run() throws Exception {
		String[] args = getScriptArgs();
		String path = args.length > 0 ? args[0] : "orig/ps2proto/pc_names.csv";
		int named = 0, skipped = 0;
		try (BufferedReader r = new BufferedReader(new FileReader(path))) {
			r.readLine(); // header
			String line;
			while ((line = r.readLine()) != null) {
				List<String> f = parseCsv(line);
				Function fn = getFunctionAt(toAddr(Long.decode(f.get(0))));
				if (fn == null) {
					skipped++;
					continue;
				}
				if (fn.getSymbol().getSource() == SourceType.USER_DEFINED) {
					skipped++;
					continue;
				}
				String name = f.get(2), object = f.get(3), signature = f.get(4);
				String method = f.get(5), evidence = f.get(6);
				Matcher m = MANGLED.matcher(name);
				if (m.matches()) {
					name = m.group(1);
				}
				Namespace ns = currentProgram.getGlobalNamespace();
				int sep = name.lastIndexOf("::");
				if (sep > 0) {
					ns = NamespaceUtils.createNamespaceHierarchy(name.substring(0, sep), null,
						currentProgram, SourceType.IMPORTED);
					name = name.substring(sep + 2);
				}
				fn.setParentNamespace(ns);
				fn.setName(name, SourceType.IMPORTED);
				String sig = signature.isEmpty() ? f.get(2) : signature;
				fn.setComment(null);
				currentProgram.getListing().setComment(fn.getEntryPoint(), CodeUnit.PLATE_COMMENT,
					String.format("PS2: %s\n%s\npaired by %s (%s)", sig, object, method, evidence));
				named++;
			}
		}
		println(String.format("named %d functions, skipped %d", named, skipped));
	}

	private static List<String> parseCsv(String line) {
		List<String> out = new ArrayList<>();
		StringBuilder cur = new StringBuilder();
		boolean quoted = false;
		for (int i = 0; i < line.length(); i++) {
			char c = line.charAt(i);
			if (quoted) {
				if (c == '"' && i + 1 < line.length() && line.charAt(i + 1) == '"') {
					cur.append('"');
					i++;
				}
				else if (c == '"') {
					quoted = false;
				}
				else {
					cur.append(c);
				}
			}
			else if (c == '"') {
				quoted = true;
			}
			else if (c == ',') {
				out.add(cur.toString());
				cur.setLength(0);
			}
			else {
				cur.append(c);
			}
		}
		out.add(cur.toString());
		return out;
	}
}
