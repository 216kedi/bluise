import argparse
import itertools
import json
from dataclasses import dataclass, field
from datetime import datetime, timezone
from typing import Dict, Iterable, List, Optional

import requests
from duckduckgo_search import DDGS
from pytrends.request import TrendReq


GOOGLE_SUGGEST_URL = "https://suggestqueries.google.com/complete/search"


DEFAULT_REGION_MAP = {
    "TR": {"geo": "TR", "hl": "tr-TR", "ddg": "tr-tr", "label": "Turkiye"},
    "DE": {"geo": "DE", "hl": "de-DE", "ddg": "de-de", "label": "Almanya"},
    "GB": {"geo": "GB", "hl": "en-GB", "ddg": "uk-en", "label": "Birlesik Krallik"},
    "US": {"geo": "US", "hl": "en-US", "ddg": "us-en", "label": "Amerika Birlesik Devletleri"},
    "FR": {"geo": "FR", "hl": "fr-FR", "ddg": "fr-fr", "label": "Fransa"},
    "IT": {"geo": "IT", "hl": "it-IT", "ddg": "it-it", "label": "Italya"},
    "ES": {"geo": "ES", "hl": "es-ES", "ddg": "es-es", "label": "Ispanya"},
    "NL": {"geo": "NL", "hl": "nl-NL", "ddg": "nl-nl", "label": "Hollanda"},
}


PYTRENDS_TRENDING_PN = {
    "TR": "turkey",
    "DE": "germany",
    "GB": "united_kingdom",
    "US": "united_states",
    "FR": "france",
    "IT": "italy",
    "ES": "spain",
    "NL": "netherlands",
}


def utc_now_iso() -> str:
    """Timezone-aware ISO 8601 UTC damgasi."""

    return datetime.now(timezone.utc).isoformat().replace("+00:00", "Z")


@dataclass
class RegionConfig:
    code: str
    geo: str
    hl: str
    ddg: str
    label: str


@dataclass
class KeywordInsight:
    query: str
    score: Optional[float] = None
    source: Optional[str] = None


@dataclass
class Competitor:
    title: str
    url: str
    snippet: Optional[str] = None


@dataclass
class RegionReport:
    region: RegionConfig
    suggestions: List[KeywordInsight] = field(default_factory=list)
    related_top: List[KeywordInsight] = field(default_factory=list)
    related_rising: List[KeywordInsight] = field(default_factory=list)
    interest_by_region: List[Dict[str, str]] = field(default_factory=list)
    trending_searches: List[KeywordInsight] = field(default_factory=list)
    competitors: List[Competitor] = field(default_factory=list)


class SEOAnalyzer:
    def __init__(
        self,
        category: str,
        region_codes: Iterable[str],
        timeframe: str = "today 12-m",
        related_limit: int = 15,
        suggest_limit: int = 20,
        competitor_limit: int = 10,
    ) -> None:
        self.category = category
        self.region_codes = list(region_codes)
        self.timeframe = timeframe
        self.related_limit = related_limit
        self.suggest_limit = suggest_limit
        self.competitor_limit = competitor_limit

    def run(self) -> Dict[str, RegionReport]:
        reports: Dict[str, RegionReport] = {}
        for region_code in self.region_codes:
            region = self._resolve_region(region_code)
            reports[region_code] = self._analyze_region(region)
        return reports

    def _resolve_region(self, region_code: str) -> RegionConfig:
        region_code = region_code.upper()
        if region_code not in DEFAULT_REGION_MAP:
            # region unknown, fall back to language neutral defaults
            return RegionConfig(
                code=region_code,
                geo=region_code,
                hl="en-US",
                ddg="wt-wt",
                label=region_code,
            )
        data = DEFAULT_REGION_MAP[region_code]
        return RegionConfig(
            code=region_code,
            geo=data["geo"],
            hl=data["hl"],
            ddg=data["ddg"],
            label=data["label"],
        )

    def _analyze_region(self, region: RegionConfig) -> RegionReport:
        report = RegionReport(region=region)

        pytrends = TrendReq(hl=region.hl, tz=0)
        self._populate_related_keywords(pytrends, region, report)
        self._populate_interest_by_region(pytrends, region, report)
        self._populate_trending_searches(pytrends, region, report)

        self._populate_google_suggestions(region, report)
        self._populate_competitors(region, report)

        return report

    def _populate_related_keywords(
        self, pytrends: TrendReq, region: RegionConfig, report: RegionReport
    ) -> None:
        try:
            pytrends.build_payload([self.category], timeframe=self.timeframe, geo=region.geo)
            related = pytrends.related_queries()
        except Exception as exc:  # pragma: no cover - ag hatalari
            report.related_top.append(
                KeywordInsight(
                    query=f"Pytrends hatasi: {exc}",
                    source="pytrends",
                )
            )
            return

        data = related.get(self.category)
        if not data:
            return

        top_df = data.get("top")
        rising_df = data.get("rising")
        if top_df is not None and not top_df.empty:
            top_records = (
                top_df.sort_values(by="value", ascending=False)
                .head(self.related_limit)
                .to_dict(orient="records")
            )
            for record in top_records:
                report.related_top.append(
                    KeywordInsight(
                        query=record.get("query"),
                        score=record.get("value"),
                        source="pytrends-top",
                    )
                )

        if rising_df is not None and not rising_df.empty:
            rising_records = (
                rising_df.sort_values(by="value", ascending=False)
                .head(self.related_limit)
                .to_dict(orient="records")
            )
            for record in rising_records:
                report.related_rising.append(
                    KeywordInsight(
                        query=record.get("query"),
                        score=record.get("value"),
                        source="pytrends-rising",
                    )
                )

    def _populate_interest_by_region(
        self, pytrends: TrendReq, region: RegionConfig, report: RegionReport
    ) -> None:
        try:
            df = pytrends.interest_by_region(resolution="country", inc_low_vol=True)
        except Exception:
            return

        if df is None or df.empty or self.category not in df.columns:
            return

        column = df[self.category]
        filtered = df[column > 0].sort_values(by=self.category, ascending=False).head(15)
        for idx, value in filtered[self.category].items():
            report.interest_by_region.append({"region": idx, "score": float(value)})

    def _populate_trending_searches(
        self, pytrends: TrendReq, region: RegionConfig, report: RegionReport
    ) -> None:
        pn = PYTRENDS_TRENDING_PN.get(region.code)
        if pn is None:
            return

        try:
            trending = pytrends.trending_searches(pn=pn)
        except Exception:
            trending = None

        if trending is None or trending.empty:
            return

        for query in trending.head(10)[0].tolist():
            report.trending_searches.append(
                KeywordInsight(query=query, source="pytrends-trending")
            )

    def _populate_google_suggestions(self, region: RegionConfig, report: RegionReport) -> None:
        terms = {self.category}
        if "hair" not in self.category.lower():
            terms.add(f"{self.category} hair transplant")
        terms.add(f"{self.category} {region.label}")

        for term in terms:
            suggestions = self._fetch_google_suggestions(term, region)
            for suggestion in suggestions:
                report.suggestions.append(
                    KeywordInsight(query=suggestion, source="google-suggest")
                )

        # Sadece ilk N benzersiz oneriyi tut
        unique_seen = {}
        for insight in report.suggestions:
            key = insight.query.lower()
            if key not in unique_seen:
                unique_seen[key] = insight
        report.suggestions = list(itertools.islice(unique_seen.values(), self.suggest_limit))

    def _fetch_google_suggestions(self, term: str, region: RegionConfig) -> List[str]:
        params = {
            "client": "firefox",
            "q": term,
            "hl": region.hl,
        }
        if region.geo:
            params["gl"] = region.geo

        try:
            response = requests.get(GOOGLE_SUGGEST_URL, params=params, timeout=10)
            response.raise_for_status()
            data = response.json()
        except Exception:
            return []

        if isinstance(data, list) and len(data) > 1:
            return [item for item in data[1] if isinstance(item, str)]
        return []

    def _populate_competitors(self, region: RegionConfig, report: RegionReport) -> None:
        queries = [
            f"{self.category} clinic {region.label}",
            f"{self.category} best hospitals {region.label}",
            f"{self.category} medical tourism {region.label}",
        ]

        with DDGS() as ddgs:
            results: List[Competitor] = []
            for query in queries:
                try:
                    ddg_results = ddgs.text(
                        query,
                        region=region.ddg,
                        safesearch="Off",
                        max_results=self.competitor_limit,
                    )
                except Exception:
                    ddg_results = []

                for entry in ddg_results:
                    title = entry.get("title") or entry.get("body") or ""
                    href = entry.get("href") or entry.get("url") or ""
                    body = entry.get("body") or entry.get("snippet")
                    if not href:
                        continue
                    results.append(Competitor(title=title, url=href, snippet=body))

        seen = {}
        for competitor in results:
            key = competitor.url.split("?")[0].lower()
            if key not in seen:
                seen[key] = competitor
        report.competitors = list(itertools.islice(seen.values(), self.competitor_limit))


def build_markdown_report(
    category: str, reports: Dict[str, RegionReport], timeframe: str
) -> str:
    lines: List[str] = []
    lines.append("# SEO Anahtar Kelime ve Rakip Analizi Raporu")
    lines.append("")
    lines.append(f"- Kategori: **{category}**")
    lines.append(f"- Zaman araligi: `{timeframe}`")
    lines.append(f"- Olusturulma: {utc_now_iso()}")
    lines.append("")

    for region_code, report in reports.items():
        lines.append(f"## Bolge: {report.region.label} ({region_code})")
        lines.append("")

        if report.suggestions:
            lines.append("**Google Suggest anahtar kelime onerileri**")
            for insight in report.suggestions:
                lines.append(f"- {insight.query}")
            lines.append("")

        if report.related_top:
            lines.append("**Google Trends iliskili sorgular (Top)**")
            for insight in report.related_top:
                score = f" (skor: {insight.score:.0f})" if insight.score is not None else ""
                lines.append(f"- {insight.query}{score}")
            lines.append("")

        if report.related_rising:
            lines.append("**Google Trends iliskili sorgular (Rising)**")
            for insight in report.related_rising:
                score = f" (skor: {insight.score:.0f})" if insight.score is not None else ""
                lines.append(f"- {insight.query}{score}")
            lines.append("")

        if report.interest_by_region:
            lines.append("**Cografi ilgi yogunlugu**")
            for item in report.interest_by_region:
                lines.append(f"- {item['region']}: skor {item['score']:.1f}")
            lines.append("")

        if report.trending_searches:
            lines.append("**Trend olan gunluk sorgular**")
            for insight in report.trending_searches:
                lines.append(f"- {insight.query}")
            lines.append("")

        if report.competitors:
            lines.append("**Rakip web siteleri (DuckDuckGo SERP)**")
            for competitor in report.competitors:
                snippet = f" - {competitor.snippet}" if competitor.snippet else ""
                lines.append(f"- [{competitor.title or competitor.url}]({competitor.url}){snippet}")
            lines.append("")

        lines.append("---")
        lines.append("")

    lines.append("_Kaynaklar: Google Suggest, Google Trends (pytrends), DuckDuckGo Search._")
    return "\n".join(lines).strip()


def build_json_report(
    category: str, reports: Dict[str, RegionReport], timeframe: str
) -> str:
    payload = {
        "category": category,
        "timeframe": timeframe,
        "generated_at": utc_now_iso(),
        "regions": {},
        "sources": ["google_suggest", "google_trends", "duckduckgo_search"],
    }

    for region_code, report in reports.items():
        payload["regions"][region_code] = {
            "label": report.region.label,
            "suggestions": [insight.__dict__ for insight in report.suggestions],
            "related_top": [insight.__dict__ for insight in report.related_top],
            "related_rising": [insight.__dict__ for insight in report.related_rising],
            "interest_by_region": report.interest_by_region,
            "trending_searches": [insight.__dict__ for insight in report.trending_searches],
            "competitors": [competitor.__dict__ for competitor in report.competitors],
        }

    return json.dumps(payload, ensure_ascii=False, indent=2)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Kategori bazli ucretsiz SEO anahtar kelime ve rakip kesif araci"
    )
    parser.add_argument("category", help="Analiz edilecek kategori veya cekirdek anahtar kelime")
    parser.add_argument(
        "--regions",
        nargs="+",
        default=["TR", "DE", "GB"],
        help="ISO ulke kodlari listesi (varsayilan: TR DE GB)",
    )
    parser.add_argument(
        "--timeframe",
        default="today 12-m",
        help="Google Trends icin zaman araligi (orn. 'now 7-d', 'today 3-m')",
    )
    parser.add_argument(
        "--related-limit",
        type=int,
        default=15,
        help="Iliskili sorgular icin maksimum kayit sayisi",
    )
    parser.add_argument(
        "--suggest-limit",
        type=int,
        default=20,
        help="Google Suggest anahtar kelime limit'i",
    )
    parser.add_argument(
        "--competitor-limit",
        type=int,
        default=10,
        help="Her bolge icin maksimum rakip sayisi",
    )
    parser.add_argument(
        "--format",
        choices=["markdown", "json"],
        default="markdown",
        help="Cikti formati",
    )
    parser.add_argument(
        "--output",
        help="Ciktinin yazilacagi dosya yolu. Bos birakilirsa stdout'a yazdirilir.",
    )

    return parser.parse_args()


def main() -> None:
    args = parse_args()
    analyzer = SEOAnalyzer(
        category=args.category,
        region_codes=args.regions,
        timeframe=args.timeframe,
        related_limit=args.related_limit,
        suggest_limit=args.suggest_limit,
        competitor_limit=args.competitor_limit,
    )

    reports = analyzer.run()
    if args.format == "json":
        output = build_json_report(args.category, reports, args.timeframe)
    else:
        output = build_markdown_report(args.category, reports, args.timeframe)

    if args.output:
        with open(args.output, "w", encoding="utf-8") as fp:
            fp.write(output)
    else:
        print(output)


if __name__ == "__main__":
    main()
