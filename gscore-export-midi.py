#!/usr/bin/env python3
import mido
import sys
import traceback
import xml.etree.ElementTree as ET

midiMessageTypes = (
    "note",
    "note_on",
    "note_off",
)
nMeasuresPerBlock = 4
ticksPerBeat = 10000


class MidiMessage:
    def __init__(self, type, pitch, velocity, time):
        self.type = type
        self.pitch = pitch
        self.velocity = velocity
        self.time = time


def main(argv):
    if len(argv) != 2:
        print(f"usage: {argv[0]} inputfile")
        return 1

    try:
        tree = ET.parse(argv[1])
    except ET.ParseError:
        print(traceback.format_exc())
        print(f"Error: Failed to parse input file '{argv[1]}'")
        return 1

    nodeGscore = tree.getroot()
    assert nodeGscore.tag == "gscore"
    assert len(nodeGscore) == 1

    nodeScore = nodeGscore[0]
    assert nodeScore.tag == "score"
    tempoBpm = int(nodeScore.attrib["tempo"])
    tempoTicks = mido.bpm2tempo(tempoBpm)
    nBeatsPerMeasure = int(nodeScore.attrib["beatspermeasure"])
    blockTimeTicks = int(
        mido.second2tick(nMeasuresPerBlock * nBeatsPerMeasure * 60.0 / tempoBpm, ticksPerBeat, tempoTicks)
    )

    nodeBlockdefs = None
    nodeTracks = None
    for node in nodeScore:
        if node.tag == "blockdefs":
            nodeBlockdefs = node
        elif node.tag == "tracks":
            nodeTracks = node
    assert nodeBlockdefs
    assert nodeTracks

    blockdefs = {}
    for iBlockdef in range(len(nodeBlockdefs)):
        assert nodeBlockdefs[iBlockdef].tag == "blockdef"
        blockdefName = nodeBlockdefs[iBlockdef].attrib["name"]
        blockdefs[blockdefName] = list(nodeBlockdefs[iBlockdef])

    tracks = [None] * len(nodeTracks)
    for iTrack in range(len(nodeTracks)):
        tracks[iTrack] = []
        assert nodeTracks[iTrack].tag == "track"
        for iBlock in range(len(nodeTracks[iTrack])):
            nodeBlock = nodeTracks[iTrack][iBlock]
            try:
                blockName = nodeBlock.attrib["name"]
            except KeyError:
                continue
            blockdef = blockdefs[blockName]
            for nodeMessage in blockdef:
                assert nodeMessage.tag == "message"
                messageType = midiMessageTypes[int(nodeMessage.attrib["type"])]
                messagePitch = int(nodeMessage.attrib["pitch"])
                messageVelocity = int(
                    127
                    * float(nodeMessage.attrib["velocity"])
                    * float(nodeBlock.attrib["velocity"])
                    * float(nodeTracks[iTrack].attrib["velocity"])
                )
                messageTime = blockTimeTicks * iBlock + int(blockTimeTicks * float(nodeMessage.attrib["time"]))
                tracks[iTrack].append(MidiMessage(messageType, messagePitch, messageVelocity, messageTime))

    midiFile = mido.MidiFile()
    midiFile.ticks_per_beat = ticksPerBeat

    metaTrack = mido.MidiTrack()
    metaTrack.append(mido.MetaMessage("time_signature", numerator=nBeatsPerMeasure, denominator=4))
    metaTrack.append(mido.MetaMessage("set_tempo", tempo=tempoTicks))
    midiFile.tracks.append(metaTrack)

    for iTrack in range(len(tracks)):
        track = tracks[iTrack]
        midiTrack = mido.MidiTrack()
        for iMessage in range(len(track)):
            messageType = track[iMessage].type
            messagePitch = track[iMessage].pitch
            messageVelocity = track[iMessage].velocity
            messageTime = track[iMessage].time
            if iMessage > 0:
                messageTime -= track[iMessage - 1].time

            midiTrack.append(
                mido.Message(messageType, channel=iTrack, note=messagePitch, velocity=messageVelocity, time=messageTime)
            )
        midiFile.tracks.append(midiTrack)

    midiFile.save(f"{argv[1]}.mid")
    print(f"Saved midi file as '{argv[1]}.mid'")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
