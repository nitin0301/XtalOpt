/**********************************************************************
  Structure - Generic wrapper for Avogadro's molecule class

  Copyright (C) 2009-2010 by David C. Lonie

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 ***********************************************************************/

#ifndef STRUCTURE_H
#define STRUCTURE_H

#include <avogadro/molecule.h>
#include <avogadro/atom.h>

#include <openbabel/math/vector3.h>
#include <openbabel/mol.h>
#include <openbabel/generic.h>

#include <QtCore/QDebug>
#include <QtCore/QDateTime>
#include <QtCore/QTextStream>

#define EV_TO_KCAL_PER_MOL 23.060538

using namespace Avogadro;

namespace GlobalSearch {

  /**
   * @class Structure structure.h <globalsearch/structure.h>
   * @brief Generic molecule object.
   * @author David C. Lonie
   *
   * The Structure class provides a generic data object for storing
   * information about a molecule. It derives from Avogadro::Molecule,
   * adding new functionality to help with common tasks during a
   * global structure search.
   */
  class Structure : public Molecule
  {
    Q_OBJECT

   public:
    /**
     * Constructor.
     *
     * @param parent The object parent.
     */
    Structure(QObject *parent = 0);

    /**
     * Copy constructor.
     */
    Structure(const Structure &other);

    /**
     * Destructor.
     */
    virtual ~Structure();

    /**
     * Assignment operator. Makes a new structure with all Structure
     * specific information copied.
     * @sa copyStructure
     */
    Structure& operator=(const Structure& other);

    /**
     * Only update the structure's atoms, bonds, and residue information
     * from other.
     * @sa operator=
     */
    Structure& copyStructure(Structure *other);

    /**
     * Enum containing possible optimization statuses.
     * @sa setStatus
     * @sa getStatus
     */
    enum State {
      /** Structure has completed all optimization steps */
      Optimized = 0,
      /** Structure has completed an optimization step but may still
       * have some to complete. getCurrentOptStep() shows the step
       * that has just completed. */
      StepOptimized,
      /** Structure is waiting to start an optimization
       * step. getCurrentOptStep() shows the step it will start
       * next. */
      WaitingForOptimization,
      /** Structure is currently queued or running an optimization
       * step on the PBS server (if applicable). */
      InProcess,
       /** Structure has just been generated, and has not yet been
        * initialized */
      Empty,
      /** The Structure has completed it's current optimization step,
       * and the results of the calculation are being transferred
       * and applied.*/
      Updating,
      /** The optimization is failing. */
      Error,
      /** The Structure has been submitted to the PBS server, but has
       * not appeared in the queue yet. */
      Submitted,
      /** The Structure has been killed before finishing all
       * optimization steps. */
      Killed,
      /** The Structure has been killed after finishing all
       * optimization steps. */
      Removed,
      /** The Structure has been found to be a duplicate of
       * another. The other structure's information can be found in
       * getDuplicateString(). */
      Duplicate,
      /** The Structure is about to restart it's current optimization
       * step. */
      Restart
    };

    /** Whether the Structure has an enthalpy value set.
     * @return true if enthalpy has been set, false otherwise
     * @sa setEnthalpy
     * @sa getEnthalpy
     * @sa setPV
     * @sa getPV
     * @sa setEnergy
     * @sa getEnergy
     */
    bool hasEnthalpy()	const {return m_hasEnthalpy;};

    /** Return the energy value of the first conformer in eV. This is
     * a convenience function.
     *
     * @note The energies of the other conformers are still available
     * using energy(int). Be aware that energy(int) returns
     * kcal/mol. The multiplicative factor EV_TO_KCAL_PER_MOL has been
     * defined to aid conversion.
     *
     * @return The energy of the first conformer in eV.
     * @sa setEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa getPV
     * @sa setEnergy
     * @sa getEnergy
     */
    double getEnergy()	const {return energy(0)/EV_TO_KCAL_PER_MOL;};

    /** Return the enthalpy value of the first conformer in eV.
     *
     * @note If the enthalpy is not set but the energy is set, this
     * function assumes that the system is at zero-pressure and
     * returns the energy.
     *
     * @return The enthalpy of the first conformer in eV.
     * @sa setEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa getPV
     * @sa setEnergy
     * @sa getEnergy
     */
    double getEnthalpy()const {if (!m_hasEnthalpy) return getEnergy(); return m_enthalpy;};

    /** Returns the value PV term from an enthalpy calculation (H = U
     * + PV) in eV.
     *
     * @return The PV term in eV.
     * @sa getEnthalpy
     * @sa setEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa setEnergy
     * @sa getEnergy
     */
    double getPV()	const {return m_PV;};

    /** Returns an energetic ranking set by setRank(uint).
     * @return the energetic ranking.
     * @sa setRank
     */
    uint getRank()	const {return m_rank;};

    /** Returns the Job ID of the Structure's current running
     * optimization. Returns zero is not running.
     * @return Job ID of the structure's optimization process.
     * @sa setJobID
     */
    uint getJobID()	const {return m_jobID;};

    /** Returns the generation number of the structure. Only useful
     * for genetic/evolutionary algorithms.
     * @return Generation number
     * @sa setGeneration
     * @sa getIDNumber
     * @sa getIndex
     * @sa setIDNumber
     * @sa setIndex
     * @sa getIDString
     */
    uint getGeneration() const {return m_generation;};

    /** Returns an ID number associated with the Structure.
     *
     * @note If a generation number is used as well, this may not be
     * unique.
     * @return Identification number
     * @sa setGeneration
     * @sa getGeneration
     * @sa getIndex
     * @sa setIDNumber
     * @sa setIndex
     * @sa getIDString
     */
    uint getIDNumber() const {return m_id;};

    /** Returns a unique ID number associated with the Structure. This
     * is typically assigned in order of introduction to a tracker.
     *
     * @return Unique identification number
     * @sa setGeneration
     * @sa getGeneration
     * @sa getIndex
     * @sa setIDNumber
     * @sa getIDNumber
     * @sa getIDString
     */
    int getIndex() const {return m_index;};

    /** @return A string naming the Structure that this Structure is a
     * duplicate of.
     * @sa setDuplicateString
     */
    QString getDuplicateString() const {return m_dupString;};

    /** @return a string describing the ancestory of the Structure.
     * @sa setParents
     */
    QString getParents() const {return m_parents;};

    /** @return The path on the remote server to write this Structure
     * for optimization.
     * @sa setRempath
     */
    QString getRempath() const {return m_rempath;};

    /** @return The current status of the Structure.
     * @sa setStatus
     * @sa State
     */
    State getStatus()	const {return m_status;};

    /** @return The current optimization step of the Structure.
     * @sa setCurrentOptStep
     */
    uint getCurrentOptStep() {return m_currentOptStep;};

    /** @return The number of times this Structure has failed the
     * current optimization step.
     * @sa setFailCount
     * @sa addFailure
     * @sa resetFailCount
     */
    uint getFailCount() { return m_failCount;};

    /** @return The time that the current optimization step started.
     * @sa getOptTimerEnd
     * @sa startOptTimer
     * @sa stopOptTimer
     * @sa setOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptElapsed
     */
    QDateTime getOptTimerStart() const {return m_optStart;};

    /** @return The time that the current optimization step ended.
     * @sa getOptTimerStart
     * @sa startOptTimer
     * @sa stopOptTimer
     * @sa setOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptElapsed
     */
    QDateTime getOptTimerEnd() const {return m_optEnd;};

    /** Returns a unique identification string. Defaults to
     * [generation]x[IDNumber]. Handy for debugging/error output.
     * @return Unique identification string.
     * @sa setGeneration
     * @sa getGeneration
     * @sa setIndex
     * @sa getIndex
     * @sa setIDNumber
     * @sa getIDNumber
     */
    QString getIDString() const {
      return tr("%1x%2").arg(getGeneration()).arg(getIDNumber());};

    /** @return A header line for a results printout
     * @sa getResultsEntry
     * @sa OptBase::save
     */
    virtual QString getResultsHeader() const {
      return QString("%1 %2 %3 %4 %5")
        .arg("Rank", 6)
        .arg("Gen", 6)
        .arg("ID", 6)
        .arg("Enthalpy", 10)
        .arg("Status", 11);};

    /** @return A structure-specific entry for a results printout
     * @sa getResultsHeader
     * @sa OptBase::save
     */
    virtual QString getResultsEntry() const;

    /** Find the smallest separation between all atoms in the
     * Structure.
     *
     * @return true if the operation makes sense for this Structure,
     * false otherwise (i.e. fewer than two atoms present)
     *
     * @param shortest An empty double to be overwritten with the
     * shortest interatomic distance.
     * @sa getNearestNeighborDistance
     * @sa getNearestNeighborHistogram
     */
    virtual bool getShortestInteratomicDistance(double & shortest) const;

    /** Find the distance to the nearest atom from a specified
     * point.
     *
     * Useful for checking if an atom will be too close to another
     * atom before adding it.
     *
     * @return true if the operation makes sense for this Structure,
     * false otherwise (i.e. fewer than one atom present)
     *
     * @param x Cartesian coordinate
     * @param y Cartesian coordinate
     * @param z Cartesian coordinate
     * @param shortest An empty double to be overwritten with the
     * nearest neighbor distance.
     * @sa getShortestInteratomicDistance
     * @sa getNearestNeighborHistogram
     */
    virtual bool getNearestNeighborDistance(double x,
                                            double y,
                                            double z,
                                            double & shortest) const;

    /** Generate data for a histogram of the distances between all
     * atoms, or between one atom and all others.
     *
     * If the parameter atom is specified, the resulting data will
     * represent the distance distribution between that atom and all
     * others. If omitted (or NULL), a histogram of all interatomic
     * distances is calculated.
     *
     * Useful for estimating the coordination number of an atom from
     * a plot.
     *
     * @warning This algorithm is not thoroughly tested and should not
     * be relied upon. It is merely an estimation.
     *
     * @return true if the operation makes sense for this Structure,
     * false otherwise (i.e. fewer than one atom present)
     *
     * @param distance List of distance values for the histogram
     * bins.
     *
     * @param frequency Number of Atoms within the corresponding
     * distance bin.
     *
     * @param min Value of starting histogram distance.
     * @param max Value of ending histogram distance.
     * @param step Increment between bins.
     * @param atom Optional: Atom to calculate distances from.
     *
     * @sa getShortestInteratomicDistance
     * @sa getNearestNeighborDistance
     */
    virtual bool getNearestNeighborHistogram(QList<double> & distance,
                                             QList<double> & frequency,
                                             double min,
                                             double max,
                                             double step,
                                             Atom *atom = 0) const;

    /** Add an atom to a random position in the Structure. If no other
     * atoms exist in the Structure, the new atom is placed at
     * (0,0,0).
     *
     * @return true if the atom was sucessfully added within the
     * specified interatomic distances.
     *
     * @param atomicNumber Atomic number of atom to add.
     *
     * @param minIAD Smallest interatomic distance allowed (NULL or
     * omit for no limit)
     *
     * @param maxIAD Largest interatomic distance allowed (NULL or
     * omit for no limit)
     *
     * @param maxAttempts Maximum number of tries before giving up.
     *
     * @param atom Returns a pointer to the new atom.
     */
    virtual bool addAtomRandomly(uint atomicNumber,
                                 double minIAD = 0.0,
                                 double maxIAD = 0.0,
                                 int maxAttempts = 1000,
                                 Atom **atom = 0);

    /** @return An alphabetized list of the atomic symbols for the
     * atomic species present in the Structure.
     * @sa getNumberOfAtomsAlpha
     */
    QList<QString> getSymbols() const;

    /** @return A list of the number of species present that
     * corresponds to the symbols listed in getSymbols().
     * @sa getSymbols
     */
    QList<uint> getNumberOfAtomsAlpha() const;

    /** @return A string formated "HH:MM:SS" indicating the amount of
     * time spent in the current optimization step
     *
     * @sa setOptTimerStart
     * @sa getOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptTimerEnd
     * @sa startOptTimer
     * @sa stopOptTimer
     */
    QString getOptElapsed() const;

    /** A "fingerprint" hash of the structure. This should be treated
     * a pure virtual for now and reimplimented when needed for
     * derived class.
     *
     * Used for checking if two Structures are similar enough to be
     * marked as duplicates.
     *
     * @return A hash of key/value pairs containing data that is
     * representative of the Structure.
     */
    virtual QHash<QString, double> getFingerprint();

    /** Sort the listed structures by their enthalpies
     *
     * @param structures List of structures to sort
     * @sa rankEnthalpies
     * @sa sortAndRankByEnthalpy
     */
    static void sortByEnthalpy(QList<Structure*> *structures);

    /** Rank the listed structures by their enthalpies
     *
     * @param structures List of structures to assign ranks
     * @sa sortEnthalpies
     * @sa sortAndRankByEnthalpy
     * @sa setRank
     * @sa getRank
     */
    static void rankByEnthalpy(const QList<Structure*> &structures);

    /** Sort and rank the listed structures by their enthalpies
     *
     * @param structures List of structures to sort and assign rank
     * @sa sortByEnthalpy
     * @sa rankEnthalpies
     * @sa setRank
     * @sa getRank
     */
    static void sortAndRankByEnthalpy(QList<Structure*> *structures);

   signals:

   public slots:

    /**
     * Write supplementary data about this Structure to a file. All
     * data that is not stored in the OpenBabel-readable optimizer
     * output file should be written here.
     *
     * If reimplementing this in a derived class, call
     * writeStructureSettings(filename) to write inherited data.
     * @param filename Filename to write data to.
     * @sa writeStructureSettings
     * @sa readSettings
     */
    virtual void writeSettings(const QString &filename) {
      writeStructureSettings(filename);};

    /**
     * Read supplementary data about this Structure from a file. All
     * data that is not stored in the OpenBabel-readable optimizer
     * output file should be read here.
     *
     * If reimplementing this in a derived class, call
     * readStructureSettings(filename) to read inherited data.
     * @param filename Filename to read data from.
     * @sa readStructureSettings
     * @sa writeSettings
     */
    virtual void readSettings(const QString &filename) {
      readStructureSettings(filename);};

    /** Set the enthalpy of the Structure.
     * @param enthalpy The Structure's enthalpy
     * @sa getEnthalpy
     */
    void setEnthalpy(double enthalpy) {m_hasEnthalpy = true; m_enthalpy = enthalpy;};

    /** Set the PV term of the Structure's enthalpy (see getPV()).
     * @param pv The PV term
     * @sa getPV
     */
    void setPV(double pv) {m_PV = pv;};

    /** Reset the Structure's enthalpy and PV term to zero and clear
     * hasEnthalpy()
     * @sa setEnthalpy
     * @sa getEnthalpy
     * @sa hasEnthalpy
     * @sa setPV
     * @sa getPV
     */
    void resetEnthalpy() {m_enthalpy=0; m_PV=0; m_hasEnthalpy=false;};

    /** Reset the Structure's energy to zero
     * @sa setEnergy
     * @sa getEnergy
     */
    void resetEnergy() {std::vector<double> E; E.push_back(0); setEnergies(E);};

    /** Determine and set the energy using a forcefield method from
     * OpenBabel.
     * @param ff A string identifying the forcefield to use (default: UFF).
     */
    void setOBEnergy(const QString &ff = QString("UFF"));

    /** Set the Structure's energetic ranking.
     * @param rank The Structure's energetic ranking.
     * @sa getRank
     */
    void setRank(uint rank) {m_rank = rank;};

    /** Set the Job ID of the current optimization process.
     * @param id The current optimization process's Job ID.
     * @sa getJobID
     */
    void setJobID(uint id) {m_jobID = id;};

    /** Set the generation number of the Structure.
     * @param gen The generation number.
     * @sa setGeneration
     * @sa getGeneration
     * @sa setIndex
     * @sa getIndex
     * @sa setIDNumber
     * @sa getIDNumber
     * @sa getIDString
     */
    void setGeneration(uint gen) {m_generation = gen;};

    /** Set the ID number associated with the Structure.
     *
     * @note If a generation number is used as well, this may not be
     * unique.
     * @return Identification number
     * @sa setGeneration
     * @sa getGeneration
     * @sa setIndex
     * @sa getIndex
     * @sa getIDNumber
     * @sa getIDString
     */
    void setIDNumber(uint id) {m_id = id;};

    /** Set a unique ID number associated with the Structure. This
     * is typically assigned in order of introduction to a tracker.
     *
     * @note If a generation number is used as well, this may not be
     * unique.
     * @param index Identification number
     * @sa setGeneration
     * @sa getGeneration
     * @sa getIndex
     * @sa setIDNumber
     * @sa getIDNumber
     * @sa getIDString
     */
    void setIndex(int index) {m_index = index;};

    /** @param p A string describing the ancestory of the Structure.
     * @sa getParents
     */
    void setParents(const QString & p) {m_parents = p;};

    /** @param p The path on the remote server to write this Structure
     * for optimization.
     * @sa getRempath
     */
    void setRempath(const QString & p) {m_rempath = p;};

    /** @param status The current status of the Structure.
     * @sa getStatus
     * @sa State
     */
    void setStatus(State status) {m_status = status;};

    /** @param i The current optimization step of the Structure.
     * @sa getCurrentOptStep
     */
    void setCurrentOptStep(uint i) {m_currentOptStep = i;};

    /** @param count The number of times this Structure has failed the
     * current optimization step.
     * @sa addFailure
     * @sa getFailCount
     * @sa resetFailCount
     */
    void setFailCount(uint count) {m_failCount = count;};

    /** Reset the number of times this Structure has failed the
     * current optimization step.
     *
     * @sa addFailure
     * @sa setFailCount
     * @sa getFailCount
     */
    void resetFailCount() {setFailCount(0);};

    /** Increase the number of times this Structure has failed the
     * current optimization step by one.
     *
     * @sa resetFailCount
     * @sa setFailCount
     * @sa getFailCount
     */
    void addFailure() {setFailCount(getFailCount() + 1);};

    /** @param s A string naming the Structure that this Structure is a
     * duplicate of.
     * @sa getDuplicateString
     */
    void setDuplicateString(const QString & s) {m_dupString = s;};

    /** Record the current time as when the current optimization
     * process started.
     *
     * @sa setOptTimerStart
     * @sa getOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptTimerEnd
     * @sa stopOptTimer
     * @sa getOptElapsed
     */
    void startOptTimer() {
      m_optStart = QDateTime::currentDateTime(); m_optEnd = QDateTime();};

    /** Record the current time as when the current optimization
     * process stopped.
     *
     * @sa setOptTimerStart
     * @sa getOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptTimerEnd
     * @sa startOptTimer
     * @sa getOptElapsed
     */
    void stopOptTimer() {
      if (m_optEnd.isNull()) m_optEnd = QDateTime::currentDateTime();};

    /** @param d The time that the current optimization
     * process started.
     *
     * @sa getOptTimerStart
     * @sa setOptTimerEnd
     * @sa getOptTimerEnd
     * @sa startOptTimer
     * @sa stopOptTimer
     * @sa getOptElapsed
     */
    void setOptTimerStart(const QDateTime &d) {m_optStart = d;};

    /** @param d The time that the current optimization
     * process stopped.
     *
     * @sa setOptTimerStart
     * @sa getOptTimerStart
     * @sa getOptTimerEnd
     * @sa startOptTimer
     * @sa stopOptTimer
     * @sa getOptElapsed
     */
    void setOptTimerEnd(const QDateTime &d) {m_optEnd = d;};

    /** Load data into Structure.
     * @attention Do not use this function in new code, as it has been
     * replaced by readSettings. Old code should be rewritten to use
     * readSettings as well.
     * @deprecated Use readSettings instead, and call this only as a
     * backup for outdates .state files
     * @param in QTextStream containing load data.
     * @sa readSettings
     */
    virtual void load(QTextStream &in);

   protected slots:
    /**
     * Write data from the Structure class to a file.
     * @param filename Filename to write data to.
     * @sa writeSettings
     * @sa readSettings
     */
    void writeStructureSettings(const QString &filename);

    /**
     * Read data concerning the Structure class from a file.
     * @param filename Filename to read data from.
     * @sa writeSettings
     * @sa readSettings
     */
    void readStructureSettings(const QString &filename);

  protected:
    bool m_hasEnthalpy;
    uint m_generation, m_id, m_rank, m_jobID, m_currentOptStep, m_failCount;
    QString m_parents, m_dupString, m_rempath;
    double m_enthalpy, m_PV;
    State m_status;
    QDateTime m_optStart, m_optEnd;
    int m_index;
  };

} // end namespace Avogadro

#endif