/*
Copyright 2010  Christian Vetter veaac.fdirct@gmail.com

This file is part of MoNav.

MoNav is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

MoNav is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with MoNav.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OSMIMPORTER_H
#define OSMIMPORTER_H

#include "interfaces/iimporter.h"
#include "ientityreader.h"
#include "oisettingsdialog.h"
#include "statickdtree.h"
#include "types.h"
#include "utils/intersection.h"

#include <QObject>
#include <QHash>
#include <cstring>

class OSMImporter : public QObject, public IImporter
{
	Q_OBJECT
	Q_INTERFACES( IImporter )

public:

	OSMImporter();
	virtual QString GetName();
	virtual void SetOutputDirectory( const QString& dir );
	virtual QWidget* GetSettings();
	virtual bool LoadSettings( QSettings* settings );
	virtual bool SaveSettings( QSettings* settings );
	virtual bool Preprocess( QString filename );
	virtual bool SetIDMap( const std::vector< NodeID >& idMap );
	virtual bool GetIDMap( std::vector< NodeID >* idMap );
	virtual bool SetEdgeIDMap( const std::vector< NodeID >& idMap );
	virtual bool GetEdgeIDMap( std::vector< NodeID >* idMap );
	virtual bool GetRoutingEdges( std::vector< RoutingEdge >* data );
	virtual bool GetRoutingEdgePaths( std::vector< RoutingNode >* data );
	virtual bool GetRoutingNodes( std::vector< RoutingNode >* data );
	virtual bool GetRoutingWayNames( std::vector< QString >* data );
	virtual bool GetRoutingWayTypes( std::vector< QString >* data );
	virtual bool GetAddressData( std::vector< Place >* dataPlaces, std::vector< Address >* dataAddresses, std::vector< UnsignedCoordinate >* dataWayBuffer, std::vector< QString >* addressNames );
	virtual bool GetBoundingBox( BoundingBox* box );
	virtual void DeleteTemporaryFiles();
	virtual ~OSMImporter();

protected:

	struct Statistics{
		NodeID numberOfNodes;
		NodeID numberOfEdges;
		NodeID numberOfWays;
		NodeID numberOfPlaces;
		NodeID numberOfOutlines;
		NodeID numberOfMaxspeed;
		NodeID numberOfZeroSpeed;
		NodeID numberOfDefaultCitySpeed;
		NodeID numberOfCityEdges;

		Statistics() {
			memset( this, 0, sizeof( Statistics ) );
		}
	};

	struct Way {
		enum {
			NotSure, Oneway, Bidirectional, Opposite
		} direction;
		double maximumSpeed;
		bool usefull;

		bool access;
		int accessPriority;

		QString name;
		int namePriority;

		QString ref;
		QString placeName;
		Place::Type placeType;

		unsigned type;
		bool roundabout;

		int addPercentage;
		int addFixed;
	};

	struct Node {
		QString name;
		int namePriority;

		unsigned population;
		Place::Type type;

		int penalty;

		bool access;
		int accessPriority;
	};

	struct Relation {

	};

	struct NodeLocation {
		// City / Town
		unsigned place;
		double distance;
		bool isInPlace: 1;
	};

	struct Location {
		QString name;
		Place::Type type;
		GPSCoordinate coordinate;
	};

	struct Outline {
		QString name;
		std::vector< DoublePoint > way;

		bool operator<( const Outline& right ) const {
			return name < right.name;
		}
	};

	class GPSMetric {
	public:
		double operator() ( const unsigned left[2], const unsigned right[2] ) {
			UnsignedCoordinate leftGPS( left[0], left[1] );
			UnsignedCoordinate rightGPS( right[0], right[1] );
			double result = leftGPS.ToGPSCoordinate().ApproximateDistance( rightGPS.ToGPSCoordinate() );
			return result * result;
		}

		double operator() ( const KDTree::BoundingBox< 2, unsigned > &box, const unsigned point[2] ) {
			unsigned nearest[2];
			for ( unsigned dim = 0; dim < 2; ++dim ) {
				if ( point[dim] < box.min[dim] )
					nearest[dim] = box.min[dim];
				else if ( point[dim] > box.max[dim] )
					nearest[dim] = box.max[dim];
				else
					nearest[dim] = point[dim];
			}
			return operator() ( point, nearest );
		}

		bool operator() ( const KDTree::BoundingBox< 2, unsigned > &box, const unsigned point[2], const double radiusSquared ) {
			unsigned farthest[2];
			for ( unsigned dim = 0; dim < 2; ++dim ) {
				if ( point[dim] < ( box.min[dim] + box.max[dim] ) / 2 )
					farthest[dim] = box.max[dim];
				else
					farthest[dim] = box.min[dim];
			}
			return operator() ( point, farthest ) <= radiusSquared;
		}
	};

	typedef KDTree::StaticKDTree< 2, unsigned, unsigned, GPSMetric > GPSTree;

	struct NodeTags {
		enum Key {
			Place = 0, Population = 1, Barrier = 2, MaxTag = 3
		};
	};

	struct WayTags {
		enum Key {
			Oneway = 0, Junction = 1, Highway = 2, Ref = 3, PlaceName = 4, Place = 5, MaxSpeed = 6, MaxTag = 7
		};
	};

	struct NodePenalty {
		unsigned id;
		int seconds;

		NodePenalty( unsigned _id, int _seconds )
		{
			id = _id;
			seconds = _seconds;
		}

		NodePenalty( unsigned _id )
		{
			id = _id;
			seconds = 0;
		}

		bool operator<( const NodePenalty& right ) const
		{
			return id < right.id;
		}
	};

	bool read( const QString& inputFilename, const QString& filename );
	void readWay( Way* way, const IEntityReader::Way& inputWay );
	void readNode( Node* node, const IEntityReader::Node& inputNode );
	Place::Type parsePlaceType( const QString& type );
	void setRequiredTags( IEntityReader* reader );

	bool preprocessData( const QString& filename );
	bool computeInCityFlags( QString filename, std::vector< NodeLocation >* nodeLocation, const std::vector< UnsignedCoordinate >& nodeCoordinates, const std::vector< UnsignedCoordinate >& outlineCoordinates );
	bool remapEdges( QString filename, const std::vector< UnsignedCoordinate >& nodeCoordinates, const std::vector< NodeLocation >& nodeLocation );

	Statistics m_statistics;
	QString m_outputDirectory;
	OISettingsDialog* m_settingsDialog;

	OISettingsDialog::Settings m_settings;
	std::vector< QString > m_kmhStrings;
	std::vector< QString > m_mphStrings;

	std::vector< int > m_wayModificatorIDs;
	std::vector< int > m_nodeModificatorIDs;

	std::vector< NodePenalty > m_penaltyNodes;
	std::vector< unsigned > m_noAccessNodes;

	std::vector< unsigned > m_usedNodes;
	std::vector< unsigned > m_routingNodes;
	std::vector< unsigned > m_outlineNodes;
	QHash< QString, unsigned > m_wayNames;
	QHash< QString, unsigned > m_wayRefs;
};

#endif // OSMIMPORTER_H
