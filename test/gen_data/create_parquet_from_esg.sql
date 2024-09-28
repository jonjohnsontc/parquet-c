-- Uses DuckDB to read in a csv input file and output it as parquet
CREATE TABLE snp_esg
(
    symbol              VARCHAR,
    full_name           VARCHAR,
    gics_sector         VARCHAR,
    gics_sub_industry   VARCHAR,
    env_score           FLOAT8,
    social_score        FLOAT8,
    gov_score           FLOAT8,
    total_esg           FLOAT8,
    highest_controversy SMALLINT,
    percentile          FLOAT8,
    rating_year         SMALLINT,
    rating_month        SMALLINT,
    market_cap          BIGINT,
    beta                FLOAT8,
    overall_risk        SMALLINT
);
COPY snp_esg FROM 'test/gen_data/sp500_esg_data.csv' (
    FORMAT CSV,
    HEADER
    );
COPY snp_esg TO 'test/gen_data/sp500_esg_data.parquet';
DROP TABLE snp_esg;