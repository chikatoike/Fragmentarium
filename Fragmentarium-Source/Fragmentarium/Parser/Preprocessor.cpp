#include "Preprocessor.h"

#include <QStringList>
#include <QRegExp>
#include <QMap>
#include <QFileInfo>
#include <QDir>


#include "../../SyntopiaCore/Exceptions/Exception.h"
#include "../../SyntopiaCore/Logging/Logging.h"
#include "../../SyntopiaCore/Math/Random.h"

using namespace SyntopiaCore::Exceptions;
using namespace SyntopiaCore::Logging;
using namespace SyntopiaCore::Math;


namespace Fragmentarium {
	namespace Parser {	

		// Helpers:
		namespace {
			float parseFloat(QString s) {
				bool succes = false;
				double to = s.toDouble(&succes);
				if (!succes	) {
					WARNING("Could not parse float: " + s);
					return 0;
				}
				return to;
			};

			void setLockType(GuiParameter* p, QString lockTypeString) {
				lockTypeString = lockTypeString.toLower().trimmed();
				LockType l;
				if (lockTypeString == "locked") {
					l = Locked;
				} else if (lockTypeString == "notlocked" || lockTypeString.isEmpty()) {
					l = NotLocked;
				} else if (lockTypeString == "notlockable") {
					l = NotLockable;
				}  else if (lockTypeString == "alwayslocked") {
					l = AlwaysLocked;
				} else {
					WARNING("Not able to parse lock type: " + lockTypeString);
				}
				p->setLockType(l);
			}

			Vector4f parseVector4f(QString s1, QString s2, QString s3, QString s4) {
				return Vector4f(parseFloat(s1), parseFloat(s2), parseFloat(s3), parseFloat(s4));
			};

			Vector3f parseVector3f(QString s1, QString s2, QString s3) {
				return Vector3f(parseFloat(s1), parseFloat(s2), parseFloat(s3));
			};

			Vector3f parseVector2f(QString s1, QString s2) {
				return Vector3f(parseFloat(s1), parseFloat(s2),0.0);
			};
		}

		FragmentSource::FragmentSource() : 
			hasPixelSizeUniform(false), 
			bufferShaderSource(0),
			clearOnChange(true),
			iterationsBetweenRedraws(0),
			subframeMax(-1)
		{
		}

		
		void Preprocessor::parseSource(FragmentSource* fs,QString input, QString originalFileName, bool dontAdd) {
			fs->sourceFileNames.append(originalFileName);
			int sf = fs->sourceFileNames.count()-1;

			QStringList in = input.split(QRegExp("\r\n|\r|\n"));
			if (!isCreatingAutoSave) in.append("#group default"); // make sure we fall back to the default group after including a file.

			QList<int> lines;
			for (int i = 0; i < in.count(); i++) lines.append(i);
			lines.append(-1);

			QList<int> source;
			for (int i = 0; i < in.count(); i++) source.append(sf);
			source.append(-1);

			QRegExp includeCommand("^#include(.*)\\s\"([^\"]+)\"\\s*$"); // Look for #include "test.frag"
			QRegExp bufferShaderCommand("^#buffershader\\s\"([^\"]+)\"\\s*$"); // Look for #buffershader "test.frag"

			for (int i = 0; i < in.count(); i++) {
				if (includeCommand.indexIn(in[i]) != -1) {	
					QString fileName =  includeCommand.cap(2);
					QString post = includeCommand.cap(1);
					if (post == "") {
					} else {
						throw Exception("'#include' expected");
					}
					QString fName = fileManager->resolveName(fileName, originalFileName);
					QFile f(fName);
					if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
						throw Exception("Unable to open: " +  fName);

					INFO("Including file: " + fName);
					QString a = f.readAll();
					parseSource(fs, a, fName, isCreatingAutoSave);
					dependencies.append(fName);
					if (!dontAdd && isCreatingAutoSave) {
						fs->lines.append(lines[i]);
						fs->sourceFile.append(source[i]);
						fs->source.append(in[i]);
					}
				} else if (bufferShaderCommand.indexIn(in[i]) != -1) {
					QString fileName =  bufferShaderCommand.cap(1);
					QString fName = fileManager->resolveName(fileName, originalFileName);
					QFile f(fName);
					if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
						throw Exception("Unable to open: " +  fName);

					INFO("Including buffershader: " + fName);
					QString a = f.readAll();
					FragmentSource bs = parse(a, fName, false, isCreatingAutoSave);
					fs->bufferShaderSource = new FragmentSource(bs);
					dependencies.append(fName);
					if (!dontAdd && isCreatingAutoSave) {
						fs->lines.append(lines[i]);
						fs->sourceFile.append(source[i]);
						fs->source.append(in[i]);
					}
				} else {
					if (!dontAdd) {
						fs->lines.append(lines[i]);
						fs->sourceFile.append(source[i]);
						fs->source.append(in[i]);
					}
				}
			}
		}

		// We leak here, but fs's are copied!
		FragmentSource::~FragmentSource() { 
			/*foreach (QFile* f, sourceFiles) delete(f);*/ 
			//delete(screenShaderSource);
		}

		FragmentSource Preprocessor::createAutosaveFragment(QString input, QString file) {
			FragmentSource fs;
			dependencies.clear();
			isCreatingAutoSave = true;

			// Run the preprocessor...
			parseSource(&fs, input, file, false);

			// And remove any presets...
			for (int i = 0; i < fs.source.count(); i++) {
				QString s = fs.source[i];
				if (s.trimmed().startsWith("#preset")) {
					s = s.remove("#preset").trimmed();
					QString name = s;
					fs.source.removeAt(i);
					fs.lines.removeAt(i);
					int j = i;
					bool foundEnd = false;
					while (j< fs.source.count()) {
						if (fs.source[j].trimmed().startsWith("#endpreset")) {
							fs.source.removeAt(j);
							fs.lines.removeAt(j);
							foundEnd = true;
							break;
						} else {
							fs.source.removeAt(j);
							fs.lines.removeAt(j);
						}
					}
					if (!foundEnd) WARNING("Did not find #endpreset");
					if (i==fs.source.count()) break;
					s = fs.source[i];
				}
			}

			return fs;
		}

		FragmentSource Preprocessor::parse(QString input, QString file, bool moveMain, bool doublify) {
			INFO("Parse: " + file);
			FragmentSource fs;

			// Step one: resolve includes:
			parseSource(&fs, input, file, false);

			// Step two: resolve magic uniforms:
			QRegExp pixelSizeCommand("^\\s*uniform\\s+vec2\\s+pixelSize.*$"); // Look for 'uniform vec2 pixelSize'
			if (fs.source.indexOf(pixelSizeCommand)!=-1) {
				fs.hasPixelSizeUniform = true;
			} 

			const QString lockTypeString = "\\s*(Locked|NotLocked|NotLockable)?\\s*.?$";

			// Look for patterns like 'uniform float varName; slider[0.1;1;2.0]'
			static QRegExp float4Slider("^\\s*uniform\\s+vec4\\s+(\\S+)\\s*;\\s*slider\\[\\((\\S+),(\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+),(\\S+)\\)\\]"+lockTypeString);
			static QRegExp float3Slider("^\\s*uniform\\s+vec3\\s+(\\S+)\\s*;\\s*slider\\[\\((\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+)\\),\\((\\S+),(\\S+),(\\S+)\\)\\]"+lockTypeString);
			static QRegExp float2Slider("^\\s*uniform\\s+vec2\\s+(\\S+)\\s*;\\s*slider\\[\\((\\S+),(\\S+)\\),\\((\\S+),(\\S+)\\),\\((\\S+),(\\S+)\\)\\]"+lockTypeString);
			static QRegExp colorChooser("^\\s*uniform\\s+vec3\\s+(\\S+)\\s*;\\s*color\\[(\\S+),(\\S+),(\\S+)\\]"+lockTypeString);
			static QRegExp floatColorChooser("^\\s*uniform\\s+vec4\\s+(\\S+)\\s*;\\s*color\\[(\\S+),(\\S+),(\\S+),(\\S+),(\\S+),(\\S+)\\]"+lockTypeString);
			static QRegExp floatSlider("^\\s*uniform\\s+float\\s+(\\S+)\\s*;\\s*slider\\[(\\S+),(\\S+),(\\S+)\\]"+lockTypeString);
			static QRegExp intSlider("^\\s*uniform\\s+int\\s+(\\S+)\\s*;\\s*slider\\[(\\S+),(\\S+),(\\S+)\\]"+lockTypeString);
			static QRegExp boolChooser("^\\s*uniform\\s+bool\\s+(\\S+)\\s*;\\s*checkbox\\[(\\S+)\\]"+lockTypeString);
			static QRegExp main("^\\s*void\\s+main\\s*\\(.*$"); 
			static QRegExp replace("^#replace\\s+\"([^\"]+)\"\\s+\"([^\"]+)\"\\s*$"); // Look for #reaplace "var1" "var2"
			static QRegExp sampler2D("^\\s*uniform\\s+sampler2D\\s+(\\S+)\\s*;\\s*file\\[(.*)\\].*$");
			static QRegExp doneClear("^\\s*#define\\s+dontclearonchange$",Qt::CaseInsensitive);
			static QRegExp iterations("^\\s*#define\\s+iterationsbetweenredraws\\s*(\\d+)\\s*$",Qt::CaseInsensitive);
			static QRegExp subframeMax("^\\s*#define\\s+subframemax\\s*(\\d+)\\s*$",Qt::CaseInsensitive);
			QString lastComment;
			QString currentGroup;
			QMap<QString, QString> replaceMap;
			bool inVertex = false;
			for (int i = 0; i < fs.source.count(); i++) {
				QString s = fs.source[i];
				if (s.trimmed().startsWith("#preset")) {
					s = s.remove("#preset").trimmed();
					QString name = s;
					QStringList preset;
					fs.source.removeAt(i);
					fs.lines.removeAt(i);
					int j = i;
					bool foundEnd = false;
					while (j< fs.source.count()) {
						if (fs.source[j].trimmed().startsWith("#endpreset")) {
							fs.source.removeAt(j);
							fs.lines.removeAt(j);
							foundEnd = true;
							break;
						} else {
							preset.append(fs.source[j]);
							fs.source.removeAt(j);
							fs.lines.removeAt(j);
						}
					}
					if (!foundEnd) WARNING("Did not find #endpreset");
					fs.presets[name] = preset.join("\n");
					if (i==fs.source.count()) break;
					s = fs.source[i];
				}

				if (!s.contains("#replace")) {
					for (QMap<QString, QString>::const_iterator it = replaceMap.constBegin(); it != replaceMap.constEnd(); ++it) 
					{
						if (s.contains(it.key())) {
							fs.source[i] = s.replace(it.key(),replaceMap[it.key()]);
							//INFO("Replacing: " + s + " --> " + fs.source[i]);
							s = fs.source[i];
						}
					}
				}

				if (s.trimmed().startsWith("#camera")) {
					fs.source[i] = "// " + s;
					QString c = s.remove("#camera");
					fs.camera = c.trimmed();
				} else if (s.trimmed().startsWith("#TexParameter")) {
					fs.source[i] = "// " + s;
					QString c = s.remove("#TexParameter").trimmed();
					QStringList l = c.split(" ", QString::SkipEmptyParts);
					if (l.count()!=3) {
						WARNING("#TexParameter expects three arguments! Found: "+ l.join(","));
					} else {
						fs.textureParams[l[0]][l[1]] = l[2];
					}
				} else if (s.trimmed().startsWith("#buffer")) {
					fs.source[i] = "// " + s;
					QString c = s.remove("#buffer");
					fs.buffer = c.trimmed();
				} else if (s.trimmed().startsWith("#donotrun")) {
					fs.source[i] = "// " + s;
				} else if (s.trimmed().startsWith("#group")) {
					fs.source[i] = "// " + s;
					QString c = s.remove("#group");
					currentGroup = c.trimmed();
				} else if (s.trimmed().startsWith("#vertex")) {
					fs.source[i] = "// " + s;
					inVertex = true;
				} else if (s.contains("#endvertex")) {
					fs.source[i] = "// " + s;
					inVertex = false;
				} else if (s.trimmed().startsWith("#info")) {
					fs.source[i] = "// " + s;
					QString c = s.remove("#info").trimmed();
					SCRIPTINFO(c);
				} else if (!inVertex && moveMain && main.indexIn(s) != -1) {
					//INFO("Found main: " + s );
					fs.source[i] = s.replace(" main", " fragmentariumMain");
				}  else if (replace.indexIn(s) != -1) {
					QString from = replace.cap(1);
					QString to = replace.cap(2);
					fs.source[i] = "//" + fs.source[i];
					//INFO("Replace rule: '" + from + "' --> '" + to + "'.");
					replaceMap[from] = to;

				} else if (sampler2D.indexIn(s) != -1) {
					QString name = sampler2D.cap(1);
					fs.source[i] = "uniform sampler2D " + name + ";";
					QString fileName = fileManager->resolveName(sampler2D.cap(2),file);

					INFO("Added texture: " + name + " -> " + fileName);
					fs.textures[name] = fileName;

					SamplerParameter* sp= new SamplerParameter(currentGroup, name, lastComment, fileName);
					setLockType(sp, "alwayslocked");
					fs.params.append(sp);
				} 
				else if (floatSlider.indexIn(s) != -1) {
					QString name = floatSlider.cap(1);
					if (doublify) {
						fs.source[i] = "uniform Float " + name + ";";
					} else {
						fs.source[i] = "uniform float " + name + ";";
					}
					QString fromS = floatSlider.cap(2);
					QString defS = floatSlider.cap(3);
					QString toS = floatSlider.cap(4);

					bool succes = false;
					double from = fromS.toDouble(&succes);
					bool succes2 = false;
					double def = defS.toDouble(&succes2);
					bool succes3 = false;
					double to = toS.toDouble(&succes3);
					if (!succes || !succes2 || !succes3) {
						WARNING("Could not parse interval for uniform: " + name);
						continue;
					}

					FloatParameter* fp= new FloatParameter(currentGroup, name, lastComment, from, to, def);
					setLockType(fp, floatSlider.cap(5));
					fs.params.append(fp);
				}
				else if (floatColorChooser.indexIn(s) != -1) {
					QString name = floatColorChooser.cap(1);
					if (doublify) {
						fs.source[i] = "uniform Vec4 " + name + ";";
					} else {
						fs.source[i] = "uniform vec4 " + name + ";";
					}
					QString fromS = floatColorChooser.cap(2);
					QString defS = floatColorChooser.cap(3);
					QString toS = floatColorChooser.cap(4);
					Vector3f defaults = parseVector3f(floatColorChooser.cap(5), floatColorChooser.cap(6), floatColorChooser.cap(7));

					bool succes = false;
					double from = fromS.toDouble(&succes);
					bool succes2 = false;
					double def = defS.toDouble(&succes2);
					bool succes3 = false;
					double to = toS.toDouble(&succes3);
					if (!succes || !succes2 || !succes3) {
						WARNING("Could not parse interval for uniform: " + name);
						continue;
					}

					FloatColorParameter* fp= new FloatColorParameter(currentGroup, name, lastComment, def, from, to, defaults);
					setLockType(fp, floatColorChooser.cap(8));
					fs.params.append(fp);
				}
				else if (float3Slider.indexIn(s) != -1) {

					QString name = float3Slider.cap(1);
					if (doublify) {
						fs.source[i] = "uniform Vec3 " + name + ";";
					} else {
						fs.source[i] = "uniform vec3 " + name + ";";
					}
					Vector3f from = parseVector3f(float3Slider.cap(2), float3Slider.cap(3), float3Slider.cap(4));
					Vector3f defaults = parseVector3f(float3Slider.cap(5), float3Slider.cap(6), float3Slider.cap(7));
					Vector3f to = parseVector3f(float3Slider.cap(8), float3Slider.cap(9), float3Slider.cap(10));

					Float3Parameter* fp= new Float3Parameter(currentGroup, name, lastComment, from, to, defaults);
					setLockType(fp, float3Slider.cap(11));
					fs.params.append(fp);
				} else if (float4Slider.indexIn(s) != -1) {

					QString name = float4Slider.cap(1);
					if (doublify) {
						fs.source[i] = "uniform Vec4 " + name + ";";
					} else {
						fs.source[i] = "uniform vec4 " + name + ";";
					}
					Vector4f from = parseVector4f(float4Slider.cap(2), float4Slider.cap(3), float4Slider.cap(4), float4Slider.cap(5));
					Vector4f defaults = parseVector4f(float4Slider.cap(6), float4Slider.cap(7), float4Slider.cap(8), float4Slider.cap(9));
					Vector4f to = parseVector4f(float4Slider.cap(10), float4Slider.cap(11), float4Slider.cap(12), float4Slider.cap(13));

					Float4Parameter* fp= new Float4Parameter(currentGroup, name, lastComment, from, to, defaults);
					setLockType(fp, float4Slider.cap(14));
					fs.params.append(fp);
				} else if (float2Slider.indexIn(s) != -1) {
					QString name = float2Slider.cap(1);
					if (doublify) {
						fs.source[i] = "uniform Vec2 " + name + ";";
					} else {
						fs.source[i] = "uniform vec2 " + name + ";";
					}
					Vector3f from = parseVector2f(float2Slider.cap(2), float2Slider.cap(3));
					Vector3f defaults = parseVector2f(float2Slider.cap(4), float2Slider.cap(5));
					Vector3f to = parseVector2f(float2Slider.cap(6), float2Slider.cap(7));

					Float2Parameter* fp= new Float2Parameter(currentGroup, name, lastComment, from, to, defaults);
					setLockType(fp, float2Slider.cap(8));
					fs.params.append(fp);
				} else if (colorChooser.indexIn(s) != -1) {
					QString name = colorChooser.cap(1);
					if (doublify) {
						fs.source[i] = "uniform Vec3 " + name + ";";
					} else {
						fs.source[i] = "uniform vec3 " + name + ";";
					}Vector3f defaults = parseVector3f(colorChooser.cap(2), colorChooser.cap(3), colorChooser.cap(4));
					ColorParameter* cp= new ColorParameter(currentGroup, name, lastComment, defaults);
					setLockType(cp, colorChooser.cap(5));
					fs.params.append(cp);
				} else if (intSlider.indexIn(s) != -1) {
					QString name = intSlider.cap(1);
					fs.source[i] = "uniform int " + name + ";";
					QString fromS = intSlider.cap(2);
					QString defS = intSlider.cap(3);
					QString toS = intSlider.cap(4);

					bool succes = false;
					int from = fromS.toInt(&succes);
					bool succes2 = false;
					int def = defS.toInt(&succes2);
					bool succes3 = false;
					int to = toS.toInt(&succes3);
					if (!succes || !succes2 || !succes3) {
						WARNING("Could not parse interval for uniform: " + name);
						continue;
					}

					IntParameter* ip= new IntParameter(currentGroup, name, lastComment, from, to, def);
					setLockType(ip, intSlider.cap(5));
					fs.params.append(ip);
				} else if (boolChooser.indexIn(s) != -1) {

					QString name = boolChooser.cap(1);
					fs.source[i] = "uniform bool " + name + ";";
					QString defS = boolChooser.cap(2).toLower().trimmed();

					bool def = false;
					if (defS == "true") {
						def = true;
					} else if (defS == "false") {
						def = false;
					} else {
						WARNING("Could not parse boolean value for uniform: " + name);
						continue;
					}

					BoolParameter* bp= new BoolParameter(currentGroup, name, lastComment, def);
					setLockType(bp, boolChooser.cap(3));
					fs.params.append(bp);
				} else if (doneClear.indexIn(s) != -1) {
					fs.clearOnChange = false;
				} else if (iterations.indexIn(s) != -1) {
					QString iterationCount = iterations.cap(1);
					bool succes = false;
					int i = iterationCount.toInt(&succes);
					if (!succes) {
						WARNING("Could not parse iterations value for 'iterationsbetweenredraws': " + iterationCount);
						continue;
					}
					fs.iterationsBetweenRedraws = i;

				} else if (subframeMax.indexIn(s) != -1) {
					QString maxCount = subframeMax.cap(1);
					bool succes = false;
					int i = maxCount.toInt(&succes);
					if (!succes) {
						WARNING("Could not parse iterations value for 'subframemax': " + maxCount);
						continue;
					}
					fs.subframeMax = i;

				}


				if (s.trimmed().startsWith("//")) {
					QString c = s.remove("//");
					lastComment = c.trimmed();
				} else {
					lastComment = "";
				}

				if (doublify) {
					fs.source[i].replace("float","double");
					fs.source[i].replace("vec2","dvec2");
					fs.source[i].replace("vec3","dvec3");
					fs.source[i].replace("vec4","dvec4");
					fs.source[i].replace("mat2","dmat2");
					fs.source[i].replace("mat3","dmat3");
					fs.source[i].replace("mat4","dmat4");
				}
				fs.source[i].replace("Float","float");
				fs.source[i].replace("Vec2","vec2");
				fs.source[i].replace("Vec3","vec3");
				fs.source[i].replace("Vec4","vec4");
				fs.source[i].replace("Mat2","mat2");
				fs.source[i].replace("Mat3","mat3");
				fs.source[i].replace("Mat4","mat4");

				if (inVertex && !fs.source[i].contains("#endvertex")) {
					fs.vertexSource.append(fs.source[i]);
					fs.source[i] = "//" + fs.source[i];
				}

			}

			// To ensure main is called as the last command.
			if (moveMain) {
				fs.source.append("");
				fs.source.append("void main() { fragmentariumMain(); }");
				fs.source.append("");
			}

			return fs;
		}
	}
}

