<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">
<xsl:output method="html"/>

<xsl:template match="/">
<xsl:apply-templates select="wfdbrecord"/>
<xsl:apply-templates select="wfdbannotationset"/>
</xsl:template>

<xsl:template match="wfdbrecord">
<html>
<head><title>
Description of record <xsl:value-of select="@name"/>
</title></head>
<body><h1>
Description of record <xsl:value-of select="@name"/>
</h1>
<h2>General information</h2>
<p>
<xsl:apply-templates select="start"/>
<xsl:apply-templates select="length"/>
<b>Sampling frequency: </b>
<xsl:value-of select="samplingfrequency"/>
samples per signal per second<br/>
<xsl:if test="counterfrequency">
<b>Counter frequency: </b>
<xsl:value-of select="counterfrequency"/>
counts per second (starting from 
<xsl:value-of select="counterfrequency/@basecount"/>)<br/>
</xsl:if>
<b>Signals: </b><xsl:value-of select="signals"/></p>

<xsl:apply-templates select="info"/>
<xsl:apply-templates select="signalfile"/>
<xsl:apply-templates select="segment"/>
</body>
</html>
</xsl:template>

<xsl:template match="wfdbannotationset">
<html>
<head><title>
Record <xsl:value-of select="@record"/>,
Annotator <xsl:value-of select="@annotator"/>
</title></head>
<body><h1>
Record <xsl:value-of select="@record"/>,
Annotator <xsl:value-of select="@annotator"/>
</h1>

<p>
<xsl:apply-templates select="start"/>
<xsl:apply-templates select="length"/>
<b>Time resolution: </b><xsl:value-of select="samplingfrequency"/>
ticks per second<br/>
<b>Annotations:</b>
<table width="90%"><tr>
<td width="20%" align="right"><b>Count</b></td>
<td width="4%"></td>
<td><b>Type (Description)</b></td>
</tr>
<xsl:apply-templates select="anntab/anntabentry"/>
<tr><td align="right"><b><xsl:value-of select="count(//annotation)"/></b></td><td></td>
<td><b>Total</b></td></tr>
</table>
</p>
<hr/>
<table width="90%"><tr>
<td width="20%" align="right"><b>Time</b></td>
<td width="4%"></td>
<td width="10%"><b>Type</b></td>
<td width="8%"><b>Sub</b></td>
<td width="8%"><b>Chan</b></td>
<td width="8%"><b>Num</b></td>
<td width="4%"></td>
<td><b>Note</b></td>
</tr>
<xsl:apply-templates select="annotation"/>
</table>

</body>
</html>
</xsl:template>

<xsl:template match="anntabentry">
<tr>
<td align="right"><xsl:value-of select="anncount"/></td>
<td></td>
<td><xsl:value-of select="anncode"/>
(<xsl:value-of select="anndescription"/>)</td>
</tr>
</xsl:template>

<xsl:template match="annotation">
<tr>
<td align="right"><xsl:value-of select="time"/></td>
<td></td>
<td><xsl:value-of select="anncode"/></td>
<td><xsl:value-of select="subtype"/></td>
<td><xsl:value-of select="chan"/></td>
<td><xsl:value-of select="num"/></td>
<td></td>
<td><xsl:value-of select="aux"/></td>
</tr>
</xsl:template>

<xsl:template match="info">
<h3>Notes</h3>
<p>
<xsl:if test="age"><b>Age: </b><xsl:value-of select="age"/>
<xsl:choose>
<xsl:when test="age/@units"> (<xsl:value-of select="age/@units"/>)</xsl:when>
<xsl:otherwise> years</xsl:otherwise>
</xsl:choose>
<br/></xsl:if>
<xsl:if test="sex"><b>Sex: </b>
<xsl:choose>
<xsl:when test="sex='M' or sex='m'">male</xsl:when>
<xsl:when test="sex='F' or sex='f'">female</xsl:when>
<xsl:otherwise><xsl:value-of select="sex"/></xsl:otherwise>
</xsl:choose>
<br/></xsl:if>
<xsl:if test="diagnosis"><b>Diagnoses: </b>
<xsl:for-each select="diagnosis">
<xsl:if test="position()&gt;1">; </xsl:if>
<xsl:value-of select="."/>
</xsl:for-each>
<br/></xsl:if>
<xsl:if test="medication"><b>Medications: </b>
<xsl:for-each select="medication">
<xsl:if test="position()&gt;1">; </xsl:if>
<xsl:value-of select="."/>
<xsl:if test="@dosage"> (<xsl:value-of select="@dosage"/>)</xsl:if>
</xsl:for-each>
<br/></xsl:if>
<xsl:for-each select="other">
<xsl:value-of select="."/><br/>
</xsl:for-each>
</p>
</xsl:template>

<xsl:template match="start">
<b>Start: </b>
<xsl:if test="year">
<xsl:value-of select="year"/>/<xsl:value-of select="format-number(month, '00')"/>/<xsl:value-of select="format-number(day, '00')"/> at </xsl:if><xsl:value-of select="format-number(hour, '00')"/>:<xsl:value-of select="format-number(minute, '00')"/>:<xsl:value-of select="format-number(second, '00')"/>
<br/>
</xsl:template>

<xsl:template match="length">
<b>Duration: </b>
<xsl:value-of select="format-number(floor(. div /wfdbrecord/samplingfrequency div 3600), '0')"/>:<xsl:value-of select="format-number(floor(. div /wfdbrecord/samplingfrequency mod 3600 div 60), '00')"/>:<xsl:value-of select="format-number(. div /wfdbrecord/samplingfrequency mod 60, '00.###')"/>
(<xsl:value-of select="."/> sample intervals)<br/>
</xsl:template>

<xsl:template match="signalfile">
<xsl:choose>
<xsl:when test="filename='[none]'">
<p><b>Each of the signals listed below appears in at least one segment
of this record.  See the details for each segment (following this
list of available signals) for additional information.</b></p> 
</xsl:when>
<xsl:otherwise>
<h2>Signal file: <xsl:value-of select="filename"/></h2>
<xsl:if test="preamblelength">
<b>Preamble: </b><xsl:value-of select="preamblelength"/> bytes<br/>
</xsl:if>
</xsl:otherwise>
</xsl:choose>

<xsl:for-each select="signal">
<h3>Signal <xsl:value-of select="position() - 1"/>: <xsl:value-of select="description"/></h3>
<p>
<b>Gain: </b><xsl:value-of select="gain"/>
adu/<xsl:value-of select="units"/><br/>
<b>Initial value: </b><xsl:value-of select="initialvalue"/> adu<br/>
<xsl:if test="storageformat!='0'">
<b>Storage format: </b><xsl:value-of select="storageformat"/><br/>
</xsl:if>
<xsl:if test="samplesperframe">
<b>Samples per frame: </b><xsl:value-of select="samplesperframe"/><br/>
</xsl:if>
<xsl:if test="skew">
<b>Skew: </b><xsl:value-of select="skew"/> sample intervals<br/>
</xsl:if>
<b>ADC resolution: </b><xsl:value-of select="adcresolution"/> bits<br/>
<xsl:if test="adczero!=0">
<b>ADC zero: </b><xsl:value-of select="adczero"/> adu<br/>
</xsl:if>
<xsl:if test="baseline!=0">
<b>Baseline: </b><xsl:value-of select="baseline"/> adu<br/>
</xsl:if>
<xsl:if test="blocksize&gt;0">
<b>Block size: </b><xsl:value-of select="blocksize"/><br/>
</xsl:if>
<xsl:if test="storageformat!='0'">
<b>Checksum: </b><xsl:value-of select="checksum"/>
</xsl:if>
</p>
</xsl:for-each>
</xsl:template>

<xsl:template match="segment">
<hr/><h2>Segment <xsl:value-of select="position()-1"/>: <xsl:value-of select="@name"/></h2>
<p>
<xsl:choose>
<xsl:when test="not(gap)">
<xsl:apply-templates select="start"/>
<xsl:apply-templates select="length"/>
<b>Sampling frequency: </b>
<xsl:value-of select="samplingfrequency"/>
samples per signal per second<br/>
<xsl:if test="counterfrequency">
<b>Counter frequency: </b>
<xsl:value-of select="counterfrequency"/>
counts per second (starting from 
<xsl:value-of select="counterfrequency/@basecount"/>)<br/>
</xsl:if>
<b>Signals: </b>
<xsl:value-of select="signals"/>
</xsl:when>
<xsl:when test="gap">
<xsl:apply-templates select="gap/start"/>
<xsl:apply-templates select="gap/length"/>
</xsl:when>
</xsl:choose>
</p>
<xsl:apply-templates select="signalfile"/>

</xsl:template>

</xsl:stylesheet>
