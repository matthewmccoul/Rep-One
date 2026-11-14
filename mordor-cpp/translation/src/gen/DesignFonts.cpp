#include "core.h"

namespace mc {

DesignFont::DesignFont(cs::String Face, double Points, bool Bold, cs::String Tag)
    : Face(Face), Points(Points), Bold(Bold), Tag(Tag) {}

DesignFont DesignFont::Plain() {
    DesignFont _o = *this;
    _o.Bold = false;
    return _o;
}

cs::Nullable<double> DesignFont::FixedPoints() {
    const cs::String& t = Tag;
    double p = 0;
    if ((int32_t)t.size() > 1 && t[0] == ':' && cs::Parse::TryParse(cs::Str::Substring(t, 1), p))
        return p;
    return {};
}

cs::String DesignFont::ToString() {
    return Face + " " + cs::ToStr(Points) + (Bold ? " bold" : "") + (!Tag.empty() ? " " + Tag : "");
}

ResolvedFont::ResolvedFont(cs::String Face, double Points) : Face(Face), Points(Points) {}

double ResolvedFont::Pixels() { return FontRule::Pixels(Points); }

bool FontRule::IsDefault(cs::Ref<GameOptions> o) {
    return o->ProportionalFont == DesignFont::ProFace && o->NonProportionalFont == DesignFont::NonProFace
        && o->ProportionalSize == GameOptions::DefaultProportionalSize && o->NonProportionalSize == GameOptions::DefaultNonProportionalSize;
}

ResolvedFont FontRule::Resolve(cs::Ref<GameOptions> o, DesignFont d, FontPass pass) {
    cs::String face = d.Face;
    double points = d.Points;
    if (pass != FontPass::SizeOnly && !IsDefault(o)) {
        // sub0784, test by test: VB's "=" on strings is Option Compare Binary, exact
        if (face == DesignFont::ProFace) { face = o->ProportionalFont; points = Sized(points, o->ProportionalSize); }
        if (face == DesignFont::NonProFace) { face = o->NonProportionalFont; points = Sized(points, o->NonProportionalSize); }
    }
    if (pass != FontPass::Restyle)
        points = Sized(points, d.FixedPoints().value_or((double)(face == o->ProportionalFont ? o->ProportionalSize : o->NonProportionalSize)));
    return ResolvedFont(face, points);
}

double FontRule::Sized(double had, double wanted) { return wanted > 0 ? wanted : had; }

double FontRule::Pixels(double points) {
    return cs::Math::Max(1.0, cs::Math::Round(points * 96 / 72, cs::MidpointRounding::AwayFromZero));
}

DesignFonts_Form::DesignFonts_Form(cs::String Name, int32_t Resource, FontPass Pass)
    : Name(Name), Resource(Resource), Pass(Pass) {}

DesignFonts_Entry DesignFonts_Form::Control(cs::String key, DesignFont font) {
    return DesignFonts_Entry(shared_from_this(), key, font);
}

DesignFonts_Entry::DesignFonts_Entry(cs::Ref<DesignFonts_Form> Form, cs::String Control, DesignFont Font)
    : Form(Form), Control(Control), Font(Font) {}

}  // namespace mc
