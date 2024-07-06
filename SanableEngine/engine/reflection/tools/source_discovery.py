from functools import cached_property
import os
from urllib import request
from config import logger
from clang.cindex import *
import zlib
import glob

index = Index.create()
additionalCompilerOptions = []

class SourceFile:
	fileTypes = {
		".h"   : "c++-header",
		".hpp" : "c++-header",
		".inl" : "c++-header",
		".c"   : "c++",
		".cpp" : "c++",

		".txt" : None,
		".xml" : None,
		".yml" : None,
		".json" : None
	}

	def __init__(this, filePath: str, project: "Project"):
		this.path = filePath.replace(os.altsep, os.sep)
		this.abspath = os.path.abspath(this.path)
		name, ext = os.path.splitext(this.path)
		this.isGenerated = name.endswith(".generated")
		this.project = project
		
		this.type = SourceFile.fileTypes[ext] \
			if ext in SourceFile.fileTypes.keys() \
			else None

		this.hasError = ext not in SourceFile.fileTypes.keys()
		this.tu: TranslationUnit = None
		
		assert os.path.exists(this.path)
		with open(this.path, "r") as f:
			this.contents = f.read()
		this.contentsHash = zlib.adler32(this.contents.encode("utf-8"))
		
		this.includes_literal = [i.strip()[len("#include <"):-1] for i in this.contents.split("\n") if i.strip().startswith("#include ")]
		this.includes_literal = [(_real if _real != None else _lit) for _real,_lit in [(project.resolveInclude(i, this.path),i) for i in this.includes_literal] ]
		
	@cached_property
	def includes(this):
		return [
			i for i in [
				next(filter(lambda f: f.path==i, this.project.files), None)
				for i in this.includes_literal
			]
			if i != None
		]

	def __repr__(this):
		return this.path

	def __eq__(this, other):
		return isinstance(other, SourceFile) and this.path == other.path

	def __hash__(this):
		return hash(this.path)

	def __getstate__(this):
		return (this.path, this.isGenerated, this.type, this.hasError, this.contentsHash, this.includes_literal)

	def __setstate__(this, d):
		(this.path, this.isGenerated, this.type, this.hasError, this.contentsHash, this.includes_literal) = d
		this.tu = None

	def owns(this, cursor: Cursor) -> bool:
		return this.path == cursor.location.file.name and cursor.is_definition()

	@cached_property
	def name(this):
		return os.path.split(this.path)[-1]

	def parse(this) -> Cursor:
		assert not this.hasError
		assert this.type != None

		if this.tu == None:
			cli_args = ["-std=c++17", "--language="+this.type, "-D__STIX_REFELCTION_GENERATING"]
			cli_args.extend(['-I'+i for i in this.project.includeDirs]) # FIXME not space safe!
			cli_args.extend(additionalCompilerOptions)
			try:
				this.tu = index.parse(this.path, args=cli_args, options=TranslationUnit.PARSE_SKIP_FUNCTION_BODIES)
			except Exception:
				logger.critical(f"Error parsing translation unit for target {this.path}", exc_info=True)
				raise

		return this.tu.cursor
	
class Project:
	def __init__(this, srcRoots: list[str], includeDirs: list[str]):
		this.files = None
		this.srcRoots = srcRoots
		this.includeDirs = includeDirs
		
	def discover(this):
		assert this.files == None, "Already enumerated files"
		this.files = []
		for p in this.srcRoots: this.files += glob.glob(p+"/**", recursive=True)
		this.files = [SourceFile(i, this) for i in sorted(set(this.files)) if not os.path.isdir(i)]
		
	def resolveInclude(this, requestedPath: str, includerPath: str):
		includerBaseDir = os.path.dirname(includerPath) if os.path.isfile(includerPath) else includerPath
		
		# Try path relative to including file
		out = os.path.join(includerBaseDir, requestedPath)
		if os.path.exists(out) and os.path.isfile(out): return out

		# Try passed include directories, in order
		for includeDir in this.includeDirs:
			out = os.path.join(includeDir, requestedPath)
			if os.path.exists(out) and os.path.isfile(out): return out

		# TODO handle system libraries?

		# Failed to locate
		return None

class ProjectDiff:
	def __init__(this, oldProj: Project, newProj: Project):
		lookupMatchingFile = lambda file, _list: next(filter(lambda i: i.path == file.path, _list), None)
		
		def isUpToDate(file: SourceFile):
			oldMatch:SourceFile = lookupMatchingFile(file, oldProj.files)
			newMatch:SourceFile = lookupMatchingFile(file, newProj.files)
			if oldMatch.contentsHash != newMatch.contentsHash: return False
			if oldMatch.includes != newMatch.includes: return False

			return all((isUpToDate(i) for i in oldMatch.includes))
			
		existedPreviously = lambda file: any((i.path == file.path for i in oldProj.files))
		existsCurrently   = lambda file: any((i.path == file.path for i in newProj.files))
		
		this.upToDate = [i for i in newProj.files if existedPreviously(i) and isUpToDate(i)]
		this.outdated = [i for i in newProj.files if existedPreviously(i) and not isUpToDate(i)]
		this.new     = [i for i in newProj.files if not existedPreviously(i)]
		this.removed = [i for i in oldProj.files if not existsCurrently(i)]
		
		pass

	def __str__(this):
		return f"{len(this.upToDate)} up-to-date | {len(this.outdated)} outdated | {len(this.new)} new | {len(this.removed)} deleted"
	